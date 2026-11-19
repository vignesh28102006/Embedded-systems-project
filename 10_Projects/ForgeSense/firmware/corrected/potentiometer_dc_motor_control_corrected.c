/*
 * ============================================================================
 * File: potentiometer_dc_motor_control_corrected.c
 * Target: STM32F401CCU6 (ARM Cortex-M4 "Black Pill")
 * Purpose: Corrected bare-metal potentiometer-controlled DC motor driver
 * Methodology: Register-level peripheral configuration (CMSIS device headers)
 * ============================================================================
 * AUDIT CORRECTIONS APPLIED:
 * 1. ADC Timeout Behavior: Original returned stale potValue on timeout,
 *    allowing the motor to continue running blindly. Fixed to assert Motor_Stop()
 *    and return 0U on timeout.
 * 2. Motor Stop Behavior: Direction pins (PA8, PA9) now explicitly de-energized
 *    at zero speed to prevent static coil heating.
 * 3. Atomic GPIO bit handling: Utilized BSRR for clean atomic setting/clearing.
 * 4. Failsafe: Checked for zero ADC reading to explicitly stop motor.
 * ============================================================================
 */

#include "stm32f4xx.h"
#include <stdint.h>

#define POT_THRESHOLD   2500U   /* Stop threshold */
#define ADC_MAX_VALUE   4095U   /* 12-bit ADC max */
#define MOTOR_PWM_MAX   1000U   /* Timer ARR = 999 (1000 counts) */

#define MOTOR_IN3_PIN   8U      /* PA8: L298N Direction IN3 */
#define MOTOR_IN4_PIN   9U      /* PA9: L298N Direction IN4 */
#define MOTOR_EN_PIN    6U      /* PA6: L298N PWM Enable (TIM3_CH1) */

volatile uint32_t potValue = 0U;
volatile uint32_t motorPWM = 0U;

void GPIO_Init(void)
{
    /* Enable GPIOA peripheral clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* PA0: Analog mode (11b), no pull-up/pull-down (00b) */
    GPIOA->MODER &= ~(3U << (0U * 2U));
    GPIOA->MODER |=  (3U << (0U * 2U));
    GPIOA->PUPDR &= ~(3U << (0U * 2U));

    /* PA8 & PA9: General purpose output mode (01b), push-pull (0b) */
    GPIOA->MODER &= ~((3U << (MOTOR_IN3_PIN * 2U)) |
                      (3U << (MOTOR_IN4_PIN * 2U)));
    GPIOA->MODER |=  ((1U << (MOTOR_IN3_PIN * 2U)) |
                      (1U << (MOTOR_IN4_PIN * 2U)));
    GPIOA->OTYPER &= ~((1U << MOTOR_IN3_PIN) | (1U << MOTOR_IN4_PIN));

    /* PA6: Alternate Function mode (10b), AF2 (TIM3_CH1) */
    GPIOA->MODER &= ~(3U << (MOTOR_EN_PIN * 2U));
    GPIOA->MODER |=  (2U << (MOTOR_EN_PIN * 2U));

    GPIOA->AFR[0] &= ~(0xFU << (MOTOR_EN_PIN * 4U));
    GPIOA->AFR[0] |=  (2U << (MOTOR_EN_PIN * 4U)); /* AF2 = TIM3_CH1 */

    GPIOA->OTYPER &= ~(1U << MOTOR_EN_PIN);

    /* Initially de-energize direction outputs */
    GPIOA->BSRR = (1U << (MOTOR_IN3_PIN + 16U)) |
                  (1U << (MOTOR_IN4_PIN + 16U));
}

void ADC_Init(void)
{
    /* Enable ADC1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* Common prescaler: PCLK2 / 2 (16 MHz / 2 = 8 MHz, max permitted is 36 MHz) */
    ADC->CCR &= ~(3U << 16U);

    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;

    /* Channel 0 sample time: 84 cycles (100b in SMPR2 bits [2:0]) */
    ADC1->SMPR2 &= ~(7U << (0U * 3U));
    ADC1->SMPR2 |=  (4U << (0U * 3U));

    /* 1 conversion in regular sequence (L[3:0] = 0000 in SQR1) */
    ADC1->SQR1 = 0U;
    ADC1->SQR2 = 0U;
    ADC1->SQR3 = 0U; /* First conversion = Channel 0 */

    /* Turn on ADC */
    ADC1->CR2 |= ADC_CR2_ADON;
}

void Motor_PWM_Init(void)
{
    /* Enable TIM3 clock on APB1 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->CR1 = 0U;
    TIM3->PSC = 15U;                /* Prescaler: 16 MHz / (15 + 1) = 1 MHz timer clock */
    TIM3->ARR = MOTOR_PWM_MAX - 1U; /* Period: 1 MHz / 1000 = 1 kHz PWM frequency */

    /* Configure CH1 as PWM Mode 1 (OC1M = 110b) with Preload Enable (OC1PE = 1) */
    TIM3->CCMR1 &= ~(7U << 4U);
    TIM3->CCMR1 |=  (6U << 4U);
    TIM3->CCMR1 |=  (1U << 3U);

    /* Channel 1 output enable, active high */
    TIM3->CCER &= ~TIM_CCER_CC1P;
    TIM3->CCER |=  TIM_CCER_CC1E;

    TIM3->CCR1 = 0U;
    TIM3->EGR |= TIM_EGR_UG;        /* Force update generation */

    /* Auto-reload preload enable, start counter */
    TIM3->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

void Motor_Stop(void)
{
    TIM3->CCR1 = 0U;
    TIM3->EGR |= TIM_EGR_UG;

    /* De-energize both H-bridge direction pins */
    GPIOA->BSRR = (1U << (MOTOR_IN3_PIN + 16U)) |
                  (1U << (MOTOR_IN4_PIN + 16U));

    motorPWM = 0U;
}

uint32_t Read_Potentiometer(void)
{
    uint32_t timeout = 1000000U;

    ADC1->SQR3 = 0U; /* Channel 0 */
    ADC1->SR &= ~ADC_SR_EOC;
    ADC1->CR2 |= ADC_CR2_SWSTART;

    while ((ADC1->SR & ADC_SR_EOC) == 0U)
    {
        if (timeout-- == 0U)
        {
            /* Safety Failsafe: shut down motor on conversion timeout */
            Motor_Stop();
            return 0U;
        }
    }

    return (ADC1->DR & ADC_MAX_VALUE);
}

void Motor_Control(uint32_t adcValue)
{
    if ((adcValue >= POT_THRESHOLD) || (adcValue == 0U))
    {
        /* Stop motor at or above threshold, or at zero reading */
        Motor_Stop();
    }
    else
    {
        /* Forward direction: IN3 = 1, IN4 = 0 */
        GPIOA->BSRR = (1U << MOTOR_IN3_PIN) |
                      (1U << (MOTOR_IN4_PIN + 16U));

        /* Speed proportional to reading: 0..2499 maps to 0..999 PWM */
        motorPWM = (adcValue * MOTOR_PWM_MAX) / POT_THRESHOLD;

        if (motorPWM >= MOTOR_PWM_MAX)
        {
            motorPWM = MOTOR_PWM_MAX - 1U;
        }

        TIM3->CCR1 = motorPWM;
    }
}

int main(void)
{
    GPIO_Init();
    ADC_Init();
    Motor_PWM_Init();

    Motor_Stop();

    while (1)
    {
        potValue = Read_Potentiometer();
        Motor_Control(potValue);

        /* Loop delay ~10 ms */
        for (volatile uint32_t i = 0U; i < 50000U; i++)
        {
            __NOP();
        }
    }
}
