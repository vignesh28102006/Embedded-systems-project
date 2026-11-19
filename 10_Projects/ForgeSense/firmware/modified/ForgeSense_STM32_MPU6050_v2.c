/*
 * ForgeSense - STM32F401 Black Pill
 * Register-level firmware with:
 * ADC sensors, SSD1306 OLED, motor PWM, LEDs, emergency stop,
 * buzzer, USART2 telemetry, and MPU6050 vibration monitoring.
 *
 * UART telemetry format remains unchanged:
 * pot,tempC,battery_mV,currentADC,speed,pwm,direction,alarm,emergency
 *
 * Existing connections:
 * PA0 potentiometer
 * PA1 LM35 temperature sensor
 * PA4 battery voltage divider
 * PA5 ACS712 current sensor
 * PA6 TIM3_CH1 PWM -> L298N ENB
 * PA8 -> L298N IN3
 * PA9 -> L298N IN4
 * PA2 USART2_TX -> ESP8266 RX0
 * PB5 normal button (active LOW)
 * PB12 emergency button (active LOW)
 * PB6 I2C1 SCL -> OLED SCL and MPU6050 SCL
 * PB7 I2C1 SDA -> OLED SDA and MPU6050 SDA
 * PB9 active-LOW buzzer
 * PB0 red LED
 * PB1 green LED
 * PB2 yellow LED
 * PC13 onboard active-LOW LED
 *
 * MPU6050:
 * VCC -> 3.3V
 * GND -> GND
 * SDA -> PB7
 * SCL -> PB6
 * AD0 -> GND (I2C address 0x68)
 *
 * Assumes HCLK/PCLK1/PCLK2 = 16 MHz.
 */

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdio.h>

/* -------------------- Configuration -------------------- */

#define POT_THRESHOLD       2500U
#define TEMP_THRESHOLD      40U
#define ADC_MAX_VALUE       4095U
#define ADC_REFERENCE_MV    3300U
#define MOTOR_PWM_MAX       1000U
#define MOTOR_FORWARD_PWM   700U
#define MOTOR_REVERSE_PWM   1000U

#define OLED_ADDR           0x3CU
#define MPU6050_ADDR        0x68U

#define LED_PIN             13U
#define BUTTON_PIN          5U
#define BUZZER_PIN          9U
#define EMERGENCY_STOP_PIN  12U
#define MOTOR_ENB_PIN       6U
#define MOTOR_IN3           8U
#define MOTOR_IN4           9U

#define RED_LED_PIN         0U
#define GREEN_LED_PIN       1U
#define YELLOW_LED_PIN      2U

#define MOTOR_STOPPED       0U
#define MOTOR_FORWARD       1U
#define MOTOR_REVERSE       2U

/*
 * Tune this threshold after observing vibrationValue in Debug mode.
 * It is a raw acceleration-change value, not a calibrated vibration unit.
 */
#define VIBRATION_THRESHOLD 2500U

/* MPU6050 registers */
#define MPU6050_REG_SMPLRT_DIV   0x19U
#define MPU6050_REG_CONFIG       0x1AU
#define MPU6050_REG_GYRO_CONFIG  0x1BU
#define MPU6050_REG_ACCEL_CONFIG 0x1CU
#define MPU6050_REG_ACCEL_XOUT_H 0x3BU
#define MPU6050_REG_PWR_MGMT_1   0x6BU
#define MPU6050_REG_WHO_AM_I     0x75U

/* -------------------- Global variables -------------------- */

volatile uint32_t potData = 0U;
volatile uint32_t tempData = 0U;
volatile uint32_t batteryADC = 0U;
volatile uint32_t currentADC = 0U;
volatile uint32_t voltage_mV = 0U;
volatile uint32_t temperatureC = 0U;
volatile uint32_t batteryVoltage_mV = 0U;
volatile int32_t current_mA = 0;

volatile uint32_t buttonState = 0U;
volatile uint32_t sensorAlarm = 0U;
volatile uint32_t emergencyStopPressed = 0U;
volatile uint32_t emergencyStopLatched = 0U;

volatile uint32_t motorPWM = 0U;
volatile uint32_t motorSpeedPercent = 0U;
volatile uint32_t motorDirection = MOTOR_STOPPED;
volatile uint32_t buzzerToggleState = 0U;

/* MPU6050 variables: add these to Keil Watch 1 */
volatile int16_t accelX = 0;
volatile int16_t accelY = 0;
volatile int16_t accelZ = 0;

volatile uint32_t vibrationValue = 0U;
volatile uint32_t vibrationAlarm = 0U;
volatile uint32_t mpu6050Connected = 0U;

/* Internal vibration-calculation variables */
static int32_t previousAccelX = 0;
static int32_t previousAccelY = 0;
static int32_t previousAccelZ = 0;
static uint32_t vibrationInitialized = 0U;

/* -------------------- Function prototypes -------------------- */

void Emergency_Stop_Check(void);
void Emergency_Stop_Execute(void);
void USART2_Init(void);
void USART2_SendTelemetry(void);
void Motor_Stop(void);

void I2C1_Init(void);
void OLED_Init(void);
void OLED_DisplaySensors(void);
void OLED_DisplayEmergencyStop(void);

void MPU6050_Init(void);
void MPU6050_ReadAcceleration(void);
void MPU6050_UpdateVibration(void);

/* -------------------- Delay -------------------- */

void delay_ms(uint32_t ms)
{
    SysTick->CTRL = 0U;
    SysTick->LOAD = 16000U - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = 5U;

    while (ms-- != 0U) {
        while ((SysTick->CTRL & (1U << 16U)) == 0U) {
            /* Wait for SysTick */
        }
    }

    SysTick->CTRL = 0U;
}

/* -------------------- GPIO -------------------- */

void GPIO_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN;

    (void)RCC->AHB1ENR;

    /* PA0, PA1, PA4, PA5 analog inputs */
    GPIOA->MODER &= ~((3U << 0U) |
                      (3U << 2U) |
                      (3U << 8U) |
                      (3U << 10U));

    GPIOA->MODER |=  ((3U << 0U) |
                      (3U << 2U) |
                      (3U << 8U) |
                      (3U << 10U));

    /* PC13 onboard active-LOW LED */
    GPIOC->MODER &= ~(3U << 26U);
    GPIOC->MODER |=  (1U << 26U);
    GPIOC->OTYPER &= ~(1U << LED_PIN);
    GPIOC->BSRR = (1U << LED_PIN);

    /* PB5 and PB12 buttons with pull-ups */
    GPIOB->MODER &= ~((3U << (BUTTON_PIN * 2U)) |
                      (3U << (EMERGENCY_STOP_PIN * 2U)));

    GPIOB->PUPDR &= ~((3U << (BUTTON_PIN * 2U)) |
                      (3U << (EMERGENCY_STOP_PIN * 2U)));

    GPIOB->PUPDR |=  ((1U << (BUTTON_PIN * 2U)) |
                      (1U << (EMERGENCY_STOP_PIN * 2U)));

    /* PB9 active-LOW buzzer: HIGH means OFF */
    GPIOB->MODER &= ~(3U << (BUZZER_PIN * 2U));
    GPIOB->MODER |=  (1U << (BUZZER_PIN * 2U));
    GPIOB->OTYPER &= ~(1U << BUZZER_PIN);
    GPIOB->BSRR = (1U << BUZZER_PIN);

    /* External LEDs PB0, PB1, PB2 */
    GPIOB->MODER &= ~((3U << (RED_LED_PIN * 2U)) |
                      (3U << (GREEN_LED_PIN * 2U)) |
                      (3U << (YELLOW_LED_PIN * 2U)));

    GPIOB->MODER |=  ((3U << (RED_LED_PIN * 2U)) |
                      (3U << (GREEN_LED_PIN * 2U)) |
                      (3U << (YELLOW_LED_PIN * 2U)));

    GPIOB->OTYPER &= ~((1U << RED_LED_PIN) |
                       (1U << GREEN_LED_PIN) |
                       (1U << YELLOW_LED_PIN));

    GPIOB->BSRR = (1U << (RED_LED_PIN + 16U)) |
                  (1U << (GREEN_LED_PIN + 16U)) |
                  (1U << (YELLOW_LED_PIN + 16U));

    /* PA8 and PA9 motor direction outputs */
    GPIOA->MODER &= ~((3U << (MOTOR_IN3 * 2U)) |
                      (3U << (MOTOR_IN4 * 2U)));

    GPIOA->MODER |=  ((1U << (MOTOR_IN3 * 2U)) |
                      (1U << (MOTOR_IN4 * 2U)));

    GPIOA->OTYPER &= ~((1U << MOTOR_IN3) |
                       (1U << MOTOR_IN4));

    GPIOA->BSRR = (1U << (MOTOR_IN3 + 16U)) |
                  (1U << (MOTOR_IN4 + 16U));

    /* PA6 = TIM3_CH1 AF2 */
    GPIOA->MODER &= ~(3U << (MOTOR_ENB_PIN * 2U));
    GPIOA->MODER |=  (2U << (MOTOR_ENB_PIN * 2U));

    GPIOA->AFR[0] &= ~(0xFU << (MOTOR_ENB_PIN * 4U));
    GPIOA->AFR[0] |=  (2U << (MOTOR_ENB_PIN * 4U));

    GPIOA->OTYPER &= ~(1U << MOTOR_ENB_PIN);
}

/* -------------------- USART2 telemetry -------------------- */

void USART2_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    GPIOA->MODER &= ~((3U << 4U) | (3U << 6U));
    GPIOA->MODER |=  ((2U << 4U) | (2U << 6U));

    GPIOA->AFR[0] &= ~((0xFU << 8U) | (0xFU << 12U));
    GPIOA->AFR[0] |=  ((7U << 8U) | (7U << 12U));

    GPIOA->OTYPER &= ~((1U << 2U) | (1U << 3U));

    USART2->CR1 = 0U;
    USART2->CR2 = 0U;
    USART2->CR3 = 0U;
    USART2->BRR = 1667U;
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void USART2_SendChar(char c)
{
    uint32_t timeout = 1000000U;

    while ((USART2->SR & USART_SR_TXE) == 0U) {
        if (timeout-- == 0U) {
            return;
        }
    }

    USART2->DR = (uint8_t)c;
}

static void USART2_SendString(const char *s)
{
    while (*s != '\0') {
        USART2_SendChar(*s++);
    }
}

void USART2_SendTelemetry(void)
{
    char line[120];
    const char *directionText;
    uint32_t alarmValue;
    int length;

    if (emergencyStopLatched != 0U) {
        directionText = "STOPPED";
    } else if (motorDirection == MOTOR_FORWARD) {
        directionText = "FORWARD";
    } else if (motorDirection == MOTOR_REVERSE) {
        directionText = "REVERSE";
    } else {
        directionText = "STOPPED";
    }

    /*
     * Preserve the existing nine-field telemetry format.
     * Vibration data is deliberately not added to the dashboard stream.
     */
    alarmValue = ((potData >= POT_THRESHOLD) ||
                  (temperatureC > TEMP_THRESHOLD)) ? 1U : 0U;

    length = snprintf(line, sizeof(line),
        "%lu,%lu,%lu,%lu,%lu,%lu,%s,%lu,%lu\r\n",
        (unsigned long)potData,
        (unsigned long)temperatureC,
        (unsigned long)batteryVoltage_mV,
        (unsigned long)currentADC,
        (unsigned long)motorSpeedPercent,
        (unsigned long)motorPWM,
        directionText,
        (unsigned long)alarmValue,
        (unsigned long)emergencyStopLatched);

    if (length > 0 && length < (int)sizeof(line)) {
        USART2_SendString(line);
    }
}

/* -------------------- ADC -------------------- */

void ADC_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    ADC->CCR &= ~(3U << 16U);

    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;

    ADC1->SMPR2 &= ~((7U << 0U) |
                     (7U << 3U) |
                     (7U << 12U) |
                     (7U << 15U));

    ADC1->SMPR2 |=  ((4U << 0U) |
                     (4U << 3U) |
                     (4U << 12U) |
                     (4U << 15U));

    ADC1->SQR1 = 0U;
    ADC1->SQR2 = 0U;
    ADC1->SQR3 = 0U;

    ADC1->CR2 |= ADC_CR2_ADON;

    delay_ms(2U);
}

uint32_t ADC_Read(uint32_t channel)
{
    uint32_t timeout = 1000000U;

    ADC1->SQR1 = 0U;
    ADC1->SQR2 = 0U;
    ADC1->SQR3 = channel & 0x1FU;

    ADC1->SR &= ~ADC_SR_EOC;
    ADC1->CR2 |= ADC_CR2_SWSTART;

    while ((ADC1->SR & ADC_SR_EOC) == 0U) {
        Emergency_Stop_Check();

        if (emergencyStopLatched != 0U) {
            return 0U;
        }

        if (timeout-- == 0U) {
            return 0U;
        }
    }

    return ADC1->DR & ADC_MAX_VALUE;
}

/* -------------------- I2C1 -------------------- */

void I2C1_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PB6 SCL and PB7 SDA: AF4, open-drain */
    GPIOB->MODER &= ~((3U << 12U) | (3U << 14U));
    GPIOB->MODER |=  ((2U << 12U) | (2U << 14U));

    GPIOB->OTYPER |= (1U << 6U) | (1U << 7U);

    GPIOB->PUPDR &= ~((3U << 12U) | (3U << 14U));

    GPIOB->AFR[0] &= ~((0xFU << 24U) | (0xFU << 28U));
    GPIOB->AFR[0] |=  ((4U << 24U) | (4U << 28U));

    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0U;

    I2C1->CR2 = 16U;
    I2C1->CCR = 80U;
    I2C1->TRISE = 17U;

    I2C1->CR1 |= I2C_CR1_PE;
}

/*
 * Write one byte to a device using a register address.
 * Returns 1 on success, 0 on timeout/NACK.
 */
static uint8_t I2C1_DeviceWrite(uint8_t address,
                               uint8_t reg,
                               uint8_t value)
{
    uint32_t timeout;

    while ((I2C1->SR2 & I2C_SR2_BUSY) != 0U) {
        if (--(timeout = 100000U) == 0U) {
            return 0U;
        }
    }

    I2C1->CR1 |= I2C_CR1_START;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_SB) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    (void)I2C1->SR1;
    I2C1->DR = (uint8_t)(address << 1U);

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_ADDR) == 0U) {
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }

        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    (void)I2C1->SR1;
    (void)I2C1->SR2;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_TXE) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    I2C1->DR = reg;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_TXE) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    I2C1->DR = value;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_BTF) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    I2C1->CR1 |= I2C_CR1_STOP;

    return 1U;
}

/*
 * Read one register from a device.
 * Returns 1 on success, 0 on timeout/NACK.
 */
static uint8_t I2C1_DeviceRead(uint8_t address,
                              uint8_t reg,
                              uint8_t *value)
{
    uint32_t timeout;

    if (value == 0) {
        return 0U;
    }

    /* Send START and device address in write mode */
    I2C1->CR1 |= I2C_CR1_START;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_SB) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    (void)I2C1->SR1;
    I2C1->DR = (uint8_t)(address << 1U);

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_ADDR) == 0U) {
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }

        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C1->CR1_STOP;
            return 0U;
        }
    }

    (void)I2C1->SR1;
    (void)I2C1->SR2;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_TXE) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    I2C1->DR = reg;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_BTF) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    /* Repeated START for read */
    I2C1->CR1 |= I2C_CR1_START;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_SB) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    (void)I2C1->SR1;
    I2C1->DR = (uint8_t)((address << 1U) | 1U);

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_ADDR) == 0U) {
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }

        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    /* Disable ACK for one-byte read */
    I2C1->CR1 &= ~I2C_CR1_ACK;
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    I2C1->CR1 |= I2C_CR1_STOP;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_RXNE) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_ACK;
            return 0U;
        }
    }

    *value = (uint8_t)I2C1->DR;

    I2C1->CR1 |= I2C_CR1_ACK;

    return 1U;
}

/* -------------------- OLED I2C -------------------- */

static uint8_t I2C1_Write(uint8_t control, uint8_t data)
{
    uint32_t timeout = 100000U;

    while ((I2C1->SR2 & I2C_SR2_BUSY) != 0U) {
        if (timeout-- == 0U) {
            return 0U;
        }
    }

    I2C1->CR1 |= I2C_CR1_START;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_SB) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    (void)I2C1->SR1;
    I2C1->DR = (uint8_t)(OLED_ADDR << 1U);

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_ADDR) == 0U) {
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }

        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    (void)I2C1->SR1;
    (void)I2C1->SR2;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_TXE) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    I2C1->DR = control;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_TXE) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    I2C1->DR = data;

    timeout = 100000U;
    while ((I2C1->SR1 & I2C_SR1_BTF) == 0U) {
        if (timeout-- == 0U) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return 0U;
        }
    }

    I2C1->CR1 |= I2C_CR1_STOP;

    return 1U;
}

static void OLED_Command(uint8_t command)
{
    (void)I2C1_Write(0x00U, command);
}

static void OLED_Data(uint8_t data)
{
    (void)I2C1_Write(0x40U, data);
}

static void OLED_SetPosition(uint8_t page, uint8_t column)
{
    OLED_Command((uint8_t)(0xB0U | (page & 0x07U)));
    OLED_Command((uint8_t)(0x00U | (column & 0x0FU)));
    OLED_Command((uint8_t)(0x10U | ((column >> 4U) & 0x0FU)));
}

/* Font indexes: 0 space, 1-26 A-Z, 27-36 digits, 37 hyphen */
static const uint8_t Font5x7[38][5] = {
    {0x00,0x00,0x00,0x00,0x00},
    {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43},
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    {0x08,0x08,0x08,0x08,0x08}
};

static void OLED_Char(char c)
{
    uint8_t index;

    if (c == ' ') {
        index = 0U;
    } else if (c >= 'A' && c <= 'Z') {
        index = (uint8_t)(c - 'A' + 1);
    } else if (c >= 'a' && c <= 'z') {
        index = (uint8_t)(c - 'a' + 1);
    } else if (c >= '0' && c <= '9') {
        index = (uint8_t)(c - '0' + 27);
    } else if (c == '-') {
        index = 37U;
    } else {
        return;
    }

    for (uint8_t i = 0U; i < 5U; i++) {
        OLED_Data(Font5x7[index][i]);
    }

    OLED_Data(0x00U);
}

static void OLED_Print(const char *s)
{
    while (*s != '\0') {
        OLED_Char(*s++);
    }
}

static void OLED_PrintInt(int32_t num)
{
    char buffer[12];
    uint8_t i = 0U;

    if (num == 0) {
        OLED_Char('0');
        return;
    }

    if (num < 0) {
        OLED_Char('-');

        if (num == (-2147483647 - 1)) {
            OLED_Print("2147483648");
            return;
        }

        num = -num;
    }

    while (num > 0 && i < sizeof(buffer)) {
        buffer[i++] = (char)((num % 10) + '0');
        num /= 10;
    }

    while (i > 0U) {
        OLED_Char(buffer[--i]);
    }
}

static void OLED_Clear(void)
{
    for (uint8_t page = 0U; page < 8U; page++) {
        OLED_SetPosition(page, 0U);

        for (uint8_t column = 0U; column < 128U; column++) {
            OLED_Data(0x00U);
        }
    }
}

void OLED_Init(void)
{
    delay_ms(100U);

    OLED_Command(0xAEU);
    OLED_Command(0xD5U);
    OLED_Command(0x80U);
    OLED_Command(0xA8U);
    OLED_Command(0x3FU);
    OLED_Command(0xD3U);
    OLED_Command(0x00U);
    OLED_Command(0x40U);
    OLED_Command(0x8DU);
    OLED_Command(0x14U);
    OLED_Command(0x20U);
    OLED_Command(0x00U);
    OLED_Command(0xA1U);
    OLED_Command(0xC8U);
    OLED_Command(0xDAU);
    OLED_Command(0x12U);
    OLED_Command(0x81U);
    OLED_Command(0x7FU);
    OLED_Command(0xD9U);
    OLED_Command(0xF1U);
    OLED_Command(0xDBU);
    OLED_Command(0x40U);
    OLED_Command(0xA4U);
    OLED_Command(0xA6U);
    OLED_Command(0xAFU);

    delay_ms(10U);
    OLED_Clear();
}

void OLED_DisplaySensors(void)
{
    OLED_SetPosition(0U, 0U);
    OLED_Print("POT ");
    OLED_PrintInt((int32_t)potData);
    OLED_Print("       ");

    OLED_SetPosition(1U, 0U);
    OLED_Print("TEMP ");
    OLED_PrintInt((int32_t)temperatureC);
    OLED_Print(" C     ");

    OLED_SetPosition(2U, 0U);
    OLED_Print("BATT ");
    OLED_PrintInt((int32_t)(batteryVoltage_mV / 100U));
    OLED_Print(" V     ");

    OLED_SetPosition(3U, 0U);
    OLED_Print("CURR ");
    OLED_PrintInt((int32_t)currentADC);
    OLED_Print(" ADC   ");

    OLED_SetPosition(4U, 0U);
    OLED_Print("VIB ");
    OLED_PrintInt((int32_t)vibrationValue);
    OLED_Print("      ");

    OLED_SetPosition(5U, 0U);

    if (mpu6050Connected == 0U) {
        OLED_Print("MPU NOT FOUND");
    } else if (vibrationAlarm != 0U) {
        OLED_Print("VIBRATION HIGH");
    } else {
        OLED_Print("VIBRATION OK");
    }

    OLED_Print("     ");
}

void OLED_DisplayEmergencyStop(void)
{
    OLED_Clear();

    OLED_SetPosition(0U, 0U);
    OLED_Print("EMERGENCY");

    OLED_SetPosition(1U, 0U);
    OLED_Print("STOP");

    OLED_SetPosition(2U, 0U);
    OLED_Print("POT 0");

    OLED_SetPosition(3U, 0U);
    OLED_Print("TEMP 0 C");

    OLED_SetPosition(4U, 0U);
    OLED_Print("BATT 0 V");

    OLED_SetPosition(5U, 0U);
    OLED_Print("CURR 0 ADC");

    OLED_SetPosition(6U, 0U);
    OLED_Print("SYSTEM STOP");
}

/* -------------------- MPU6050 -------------------- */

void MPU6050_Init(void)
{
    uint8_t whoAmI = 0U;

    mpu6050Connected = 0U;

    /*
     * WHO_AM_I normally returns 0x68.
     * This check confirms whether a device responds at address 0x68.
     */
    if (I2C1_DeviceRead(MPU6050_ADDR,
                       MPU6050_REG_WHO_AM_I,
                       &whoAmI) == 0U) {
        return;
    }

    if (whoAmI != 0x68U) {
        return;
    }

    /* Wake the MPU6050: clear sleep bit */
    if (I2C1_DeviceWrite(MPU6050_ADDR,
                         MPU6050_REG_PWR_MGMT_1,
                         0x00U) == 0U) {
        return;
    }

    delay_ms(100U);

    /* Sample-rate divider */
    if (I2C1_DeviceWrite(MPU6050_ADDR,
                         MPU6050_REG_SMPLRT_DIV,
                         0x07U) == 0U) {
        return;
    }

    /* DLPF configuration */
    if (I2C1_DeviceWrite(MPU6050_ADDR,
                         MPU6050_REG_CONFIG,
                         0x06U) == 0U) {
        return;
    }

    /* Gyroscope full scale: +/- 250 degrees/sec */
    if (I2C1_DeviceWrite(MPU6050_ADDR,
                         MPU6050_REG_GYRO_CONFIG,
                         0x00U) == 0U) {
        return;
    }

    /* Accelerometer full scale: +/- 2 g */
    if (I2C1_DeviceWrite(MPU6050_ADDR,
                         MPU6050_REG_ACCEL_CONFIG,
                         0x00U) == 0U) {
        return;
    }

    mpu6050Connected = 1U;
}

/*
 * Read the six acceleration bytes.
 * Each axis is a signed 16-bit raw reading.
 */
void MPU6050_ReadAcceleration(void)
{
    uint8_t data[6];
    uint8_t reg;
    uint32_t timeout;
    uint8_t i;

    if (mpu6050Connected == 0U) {
        return;
    }

    /*
     * Read six consecutive registers. Each byte read is handled
     * individually to keep the implementation register-level.
     */
    for (i = 0U; i < 6U; i++) {
        reg = (uint8_t)(MPU6050_REG_ACCEL_XOUT_H + i);

        timeout = 100000U;
        while ((I2C1->SR2 & I2C_SR2_BUSY) != 0U) {
            if (timeout-- == 0U) {
                return;
            }
        }

        if (I2C1_DeviceRead(MPU6050_ADDR, reg, &data[i]) == 0U) {
            mpu6050Connected = 0U;
            return;
        }
    }

    accelX = (int16_t)(((uint16_t)data[0] << 8U) | data[1]);
    accelY = (int16_t)(((uint16_t)data[2] << 8U) | data[3]);
    accelZ = (int16_t)(((uint16_t)data[4] << 8U) | data[5]);
}

/*
 * Vibration approximation:
 * Sum the absolute changes in raw acceleration since the previous sample.
 *
 * This is useful for a simple prototype but is not a calibrated
 * vibration measurement in g, mm/s, or mm/s^2.
 */
void MPU6050_UpdateVibration(void)
{
    int32_t dx;
    int32_t dy;
    int32_t dz;
    uint32_t total;

    if (mpu6050Connected == 0U) {
        vibrationValue = 0U;
        vibrationAlarm = 0U;
        vibrationInitialized = 0U;
        return;
    }

    MPU6050_ReadAcceleration();

    if (mpu6050Connected == 0U) {
        vibrationValue = 0U;
        vibrationAlarm = 0U;
        vibrationInitialized = 0U;
        return;
    }

    if (vibrationInitialized == 0U) {
        previousAccelX = accelX;
        previousAccelY = accelY;
        previousAccelZ = accelZ;

        vibrationValue = 0U;
        vibrationAlarm = 0U;
        vibrationInitialized = 1U;
        return;
    }

    dx = (int32_t)accelX - previousAccelX;
    dy = (int32_t)accelY - previousAccelY;
    dz = (int32_t)accelZ - previousAccelZ;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dz < 0) dz = -dz;

    total = (uint32_t)dx + (uint32_t)dy + (uint32_t)dz;

    vibrationValue = total;
    vibrationAlarm = (vibrationValue >= VIBRATION_THRESHOLD) ? 1U : 0U;

    previousAccelX = accelX;
    previousAccelY = accelY;
    previousAccelZ = accelZ;
}

/* -------------------- Emergency stop -------------------- */

void Emergency_Stop_Check(void)
{
    if ((GPIOB->IDR & (1U << EMERGENCY_STOP_PIN)) == 0U) {
        emergencyStopPressed = 1U;
        emergencyStopLatched = 1U;
    }
}

/* -------------------- Sensor acquisition -------------------- */

void Read_Sensors(void)
{
    if (emergencyStopLatched != 0U) {
        return;
    }

    potData = ADC_Read(0U);
    if (emergencyStopLatched != 0U) return;

    tempData = ADC_Read(1U);
    if (emergencyStopLatched != 0U) return;

    batteryADC = ADC_Read(4U);
    if (emergencyStopLatched != 0U) return;

    currentADC = ADC_Read(5U);
    if (emergencyStopLatched != 0U) return;

    voltage_mV = (tempData * ADC_REFERENCE_MV) / ADC_MAX_VALUE;
    temperatureC = voltage_mV / 10U;

    batteryVoltage_mV = (batteryADC * 6600U) / ADC_MAX_VALUE;

    /* ACS712 calibration not implemented */
    current_mA = 0;

    buttonState = ((GPIOB->IDR & (1U << BUTTON_PIN)) == 0U) ? 1U : 0U;

    /* Preserve existing alarm behavior */
    sensorAlarm = ((potData >= POT_THRESHOLD) ||
                   (temperatureC > TEMP_THRESHOLD)) ? 1U : 0U;
}

/* -------------------- LEDs and buzzer -------------------- */

void External_LED_Control(void)
{
    if (emergencyStopLatched != 0U) {
        GPIOB->BSRR = (1U << (GREEN_LED_PIN + 16U)) |
                      (1U << (YELLOW_LED_PIN + 16U));
        return;
    }

    GPIOB->BSRR = (1U << (RED_LED_PIN + 16U));

    if ((potData >= POT_THRESHOLD) ||
        (temperatureC > TEMP_THRESHOLD) ||
        (vibrationAlarm != 0U)) {

        GPIOB->BSRR = (1U << YELLOW_LED_PIN) |
                      (1U << (GREEN_LED_PIN + 16U));
    } else {
        GPIOB->BSRR = (1U << GREEN_LED_PIN) |
                      (1U << (YELLOW_LED_PIN + 16U));
    }
}

void LED_Control(void)
{
    if (emergencyStopLatched != 0U) {
        return;
    }

    if ((potData <= POT_THRESHOLD) ||
        (temperatureC <= TEMP_THRESHOLD)) {

        GPIOC->BSRR = (1U << LED_PIN);
    } else if (buttonState != 0U) {
        GPIOC->BSRR = (1U << (LED_PIN + 16U));
    } else {
        GPIOC->ODR ^= (1U << LED_PIN);
    }
}

void Buzzer_Control(void)
{
    if (emergencyStopLatched != 0U) {
        GPIOB->BSRR = (1U << BUZZER_PIN);
        return;
    }

    /*
     * Existing pot alarm remains, and the vibration alarm can
     * also activate the buzzer.
     */
    if ((potData > POT_THRESHOLD) || (vibrationAlarm != 0U)) {
        buzzerToggleState ^= 1U;

        if (buzzerToggleState != 0U) {
            GPIOB->BSRR = (1U << (BUZZER_PIN + 16U));
        } else {
            GPIOB->BSRR = (1U << BUZZER_PIN);
        }
    } else {
        GPIOB->BSRR = (1U << BUZZER_PIN);
        buzzerToggleState = 0U;
    }
}

/* -------------------- Motor PWM -------------------- */

void Motor_PWM_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->CR1 = 0U;
    TIM3->PSC = 15U;
    TIM3->ARR = MOTOR_PWM_MAX - 1U;

    TIM3->CCMR1 &= ~(7U << 4U);
    TIM3->CCMR1 |= (6U << 4U);
    TIM3->CCMR1 |= (1U << 3U);

    TIM3->CCER &= ~TIM_CCER_CC1P;
    TIM3->CCER |= TIM_CCER_CC1E;

    TIM3->CCR1 = 0U;
    TIM3->EGR |= TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

void Motor_Stop(void)
{
    TIM3->CCR1 = 0U;
    TIM3->EGR |= TIM_EGR_UG;

    GPIOA->BSRR = (1U << (MOTOR_IN3 + 16U)) |
                  (1U << (MOTOR_IN4 + 16U));

    motorPWM = 0U;
    motorSpeedPercent = 0U;
    motorDirection = MOTOR_STOPPED;
}

void Motor_SetSpeed(uint32_t adcValue)
{
    if (emergencyStopLatched != 0U) {
        Motor_Stop();
        return;
    }

    TIM3->CCR1 = 0U;
    TIM3->EGR |= TIM_EGR_UG;

    if (adcValue < POT_THRESHOLD) {
        GPIOA->BSRR = (1U << MOTOR_IN3) |
                      (1U << (MOTOR_IN4 + 16U));

        motorDirection = MOTOR_FORWARD;
        motorPWM = MOTOR_FORWARD_PWM;
        motorSpeedPercent = 70U;
        TIM3->CCR1 = MOTOR_FORWARD_PWM;
    } else {
        GPIOA->BSRR = (1U << (MOTOR_IN3 + 16U)) |
                      (1U << MOTOR_IN4);

        motorDirection = MOTOR_REVERSE;
        motorPWM = MOTOR_REVERSE_PWM;
        motorSpeedPercent = 100U;
        TIM3->CCR1 = MOTOR_REVERSE_PWM;
    }
}

/* -------------------- Emergency stop behavior -------------------- */

void Emergency_Stop_Execute(void)
{
    Motor_Stop();

    GPIOB->BSRR = (1U << BUZZER_PIN);

    GPIOB->BSRR = (1U << (GREEN_LED_PIN + 16U)) |
                  (1U << (YELLOW_LED_PIN + 16U));

    potData = 0U;
    tempData = 0U;
    batteryADC = 0U;
    currentADC = 0U;
    voltage_mV = 0U;
    temperatureC = 0U;
    batteryVoltage_mV = 0U;
    current_mA = 0;
    sensorAlarm = 0U;

    OLED_DisplayEmergencyStop();

    while (1) {
        GPIOB->ODR ^= (1U << RED_LED_PIN);
        GPIOC->ODR ^= (1U << LED_PIN);

        USART2_SendTelemetry();

        delay_ms(200U);
    }
}

/* -------------------- Main -------------------- */

int main(void)
{
    GPIO_Init();
    USART2_Init();
    ADC_Init();
    I2C1_Init();
    OLED_Init();
    Motor_PWM_Init();

    /*
     * Initialize MPU6050 after I2C1 and OLED initialization.
     * The OLED and MPU6050 share PB6/PB7.
     */
    MPU6050_Init();

    Motor_Stop();

    GPIOB->BSRR = (1U << BUZZER_PIN);
    GPIOC->BSRR = (1U << LED_PIN);

    Emergency_Stop_Check();

    if (emergencyStopLatched != 0U) {
        Emergency_Stop_Execute();
    }

    while (1) {
        Emergency_Stop_Check();

        if (emergencyStopLatched != 0U) {
            Emergency_Stop_Execute();
        }

        Read_Sensors();

        Emergency_Stop_Check();

        if (emergencyStopLatched != 0U) {
            Emergency_Stop_Execute();
        }

        /* Update MPU6050 readings for Debug Watch */
        MPU6050_UpdateVibration();

        Motor_SetSpeed(potData);

        External_LED_Control();
        LED_Control();
        Buzzer_Control();
        OLED_DisplaySensors();

        /* Keep original dashboard telemetry unchanged */
        USART2_SendTelemetry();

        delay_ms(200U);
    }
}
