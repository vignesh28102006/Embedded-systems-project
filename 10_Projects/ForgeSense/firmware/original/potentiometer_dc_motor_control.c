#include "stm32f4xx.h"
#include <stdint.h>

#define POT_THRESHOLD 2500U
#define ADC_MAX_VALUE 4095U
#define MOTOR_PWM_MAX 1000U

#define MOTOR_IN3_PIN 8U
#define MOTOR_IN4_PIN 9U

volatile uint32_t potValue = 0;
volatile uint32_t motorPWM = 0;

void GPIO_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* PA0: Potentiometer analog input */
    GPIOA->MODER &= ~(3U << (0U * 2U));
    GPIOA->MODER |=  (3U << (0U * 2U));
    GPIOA->PUPDR &= ~(3U << (0U * 2U));

    /* PA8 and PA9: Motor direction outputs */
    GPIOA->MODER &= ~((3U << (8U * 2U)) |
                      (3U << (9U * 2U)));

    GPIOA->MODER |=  ((1U << (8U * 2U)) |
                      (1U << (9U * 2U)));

    GPIOA->OTYPER &= ~((1U << 8U) | (1U << 9U));

    /* PA6: TIM3_CH1 PWM output */
    GPIOA->MODER &= ~(3U << (6U * 2U));
    GPIOA->MODER |=  (2U << (6U * 2U));

    GPIOA->AFR[0] &= ~(0xFU << (6U * 4U));
    GPIOA->AFR[0] |=  (2U << (6U * 4U));

    GPIOA->OTYPER &= ~(1U << 6U);

    /* Initially stop the motor */
    GPIOA->ODR &= ~((1U << 8U) | (1U << 9U));
}

void ADC_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    ADC->CCR &= ~(3U << 16U);

    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;

    ADC1->SMPR2 &= ~(7U << (0U * 3U));
    ADC1->SMPR2 |=  (4U << (0U * 3U));

    ADC1->SQR1 = 0U;
    ADC1->SQR2 = 0U;
    ADC1->SQR3 = 0U;

    ADC1->CR2 |= ADC_CR2_ADON;
}

uint32_t Read_Potentiometer(void)
{
    uint32_t timeout = 1000000U;

    ADC1->SQR1 = 0U;
    ADC1->SQR3 = 0U;

    ADC1->SR &= ~ADC_SR_EOC;
    ADC1->CR2 |= ADC_CR2_SWSTART;

    while ((ADC1->SR & ADC_SR_EOC) == 0U)
    {
        if (timeout-- == 0U)
        {
            return potValue;
        }
    }

    return ADC1->DR & ADC_MAX_VALUE;
}

void Motor_PWM_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->CR1 = 0U;
    TIM3->PSC = 15U;
    TIM3->ARR = MOTOR_PWM_MAX - 1U;

    TIM3->CCMR1 &= ~(7U << 4U);
    TIM3->CCMR1 |=  (6U << 4U);
    TIM3->CCMR1 |=  (1U << 3U);

    TIM3->CCER &= ~TIM_CCER_CC1P;
    TIM3->CCER |= TIM_CCER_CC1E;

    TIM3->CCR1 = 0U;
    TIM3->EGR |= TIM_EGR_UG;

    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->CR1 |= TIM_CR1_CEN;
}

void Motor_Stop(void)
{
    TIM3->CCR1 = 0U;
    TIM3->EGR |= TIM_EGR_UG;

    GPIOA->ODR &= ~((1U << MOTOR_IN3_PIN) |
                    (1U << MOTOR_IN4_PIN));

    motorPWM = 0U;
}

void Motor_Control(uint32_t adcValue)
{
    if (adcValue >= POT_THRESHOLD)
    {
        /* Stop motor at or above threshold */
        Motor_Stop();
    }
    else
    {
        /* Forward direction */
        GPIOA->ODR |= (1U << MOTOR_IN3_PIN);
        GPIOA->ODR &= ~(1U << MOTOR_IN4_PIN);

        /* Speed proportional to potentiometer reading */
        motorPWM = (adcValue * MOTOR_PWM_MAX) /
                   POT_THRESHOLD;

        if (motorPWM > MOTOR_PWM_MAX)
        {
            motorPWM = MOTOR_PWM_MAX;
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

        for (volatile uint32_t i = 0U; i < 50000U; i++)
        {
            __NOP();
        }
    }
}
