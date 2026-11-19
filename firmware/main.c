/*
 * ============================================================================
 * Project: ForgeSense - Final Integrated Firmware
 * Target: STM32F401CCU6 (ARM Cortex-M4 @ 16 MHz HSI)
 * Programming Approach: Bare-metal register-level peripheral control
 * ============================================================================
 * RECONCILED HARDWARE INTERCONNECTS:
 * - PA0: Potentiometer (ADC1_IN0, Speed Setpoint)
 * - PA1: LM35 Temperature Sensor (ADC1_IN1)
 * - PA4: Battery Voltage Divider (ADC1_IN4, 2x Scaling)
 * - PA5: ACS712 Current Sensor (ADC1_IN5)
 * - PA6: TIM3_CH1 PWM (L298N Motor Speed Control, 1 kHz)
 * - PA8: L298N Direction 1 (IN3)
 * - PA9: L298N Direction 2 (IN4)
 * - PA2: USART2_TX (AF7) -> ESP8266 RX0 (9600 Baud Telemetry)
 * - PA3: USART2_RX (AF7)
 * - PB6: I2C1_SCL (SSD1306 OLED + MPU6050 IMU, 100 kHz)
 * - PB7: I2C1_SDA (SSD1306 OLED + MPU6050 IMU, 100 kHz)
 * - PB0: External Red LED (Fault / Lockout)
 * - PB1: External Green LED (Normal Operation)
 * - PB2: External Yellow LED (Warning)
 * - PB9: Active-LOW Buzzer (HIGH = Silent, LOW = Active)
 * - PB5: Operator Button (Active-LOW with pull-up)
 * - PB12: Emergency Stop Interlock Button (Active-LOW with pull-up)
 * - PC13: Onboard Status LED (Active-LOW, 5 Hz emergency blink)
 * ============================================================================
 */

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdio.h>

/* --- Configuration Constants --- */
#define POT_THRESHOLD       2500U
#define TEMP_THRESHOLD      40U
#define ADC_MAX_VALUE       4095U
#define ADC_REFERENCE_MV    3300U
#define MOTOR_PWM_MAX       1000U
#define MOTOR_FORWARD_PWM   700U
#define MOTOR_REVERSE_PWM   1000U

#define OLED_ADDR           0x3CU
#define MPU6050_ADDR        0x68U

#define LED_PIN             13U     /* PC13 onboard LED */
#define BUTTON_PIN          5U      /* PB5 normal button */
#define BUZZER_PIN          9U      /* PB9 active-LOW buzzer */
#define EMERGENCY_STOP_PIN  12U     /* PB12 emergency stop */
#define MOTOR_ENB_PIN       6U      /* PA6 TIM3_CH1 PWM */
#define MOTOR_IN3           8U      /* PA8 direction IN3 */
#define MOTOR_IN4           9U      /* PA9 direction IN4 */

#define RED_LED_PIN         0U      /* PB0 Red LED */
#define GREEN_LED_PIN       1U      /* PB1 Green LED */
#define YELLOW_LED_PIN      2U      /* PB2 Yellow LED */

#define MOTOR_STOPPED       0U
#define MOTOR_FORWARD       1U
#define MOTOR_REVERSE       2U

#define VIBRATION_THRESHOLD 2500U

/* MPU6050 Registers */
#define MPU6050_REG_SMPLRT_DIV   0x19U
#define MPU6050_REG_CONFIG       0x1AU
#define MPU6050_REG_GYRO_CONFIG  0x1BU
#define MPU6050_REG_ACCEL_CONFIG 0x1CU
#define MPU6050_REG_ACCEL_XOUT_H 0x3BU
#define MPU6050_REG_PWR_MGMT_1   0x6BU
#define MPU6050_REG_WHO_AM_I     0x75U

/* --- Global State Variables --- */
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

volatile int16_t accelX = 0;
volatile int16_t accelY = 0;
volatile int16_t accelZ = 0;

volatile uint32_t vibrationValue = 0U;
volatile uint32_t vibrationAlarm = 0U;
volatile uint32_t mpu6050Connected = 0U;

static int32_t previousAccelX = 0;
static int32_t previousAccelY = 0;
static int32_t previousAccelZ = 0;
static uint32_t vibrationInitialized = 0U;

/* --- Function Prototypes --- */
void delay_ms(uint32_t ms);
void GPIO_Init(void);
void USART2_Init(void);
void USART2_SendTelemetry(void);
void ADC_Init(void);
uint32_t ADC_Read(uint32_t channel);
void Motor_PWM_Init(void);
void Motor_Stop(void);
void Motor_SetSpeed(uint32_t adcValue);
void I2C1_Init(void);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_DisplaySensors(void);
void OLED_DisplayEmergencyStop(void);
void MPU6050_Init(void);
void MPU6050_UpdateVibration(void);
void Read_Sensors(void);
void External_LED_Control(void);
void LED_Control(void);
void Buzzer_Control(void);
void Emergency_Stop_Check(void);
void Emergency_Stop_Execute(void);

/* --- SysTick Delay (16 MHz Core Clock) --- */
void delay_ms(uint32_t ms)
{
    SysTick->CTRL = 0U;
    SysTick->LOAD = 16000U - 1U; /* 1 ms @ 16 MHz */
    SysTick->VAL = 0U;
    SysTick->CTRL = 5U;          /* CLKSOURCE = 1, ENABLE = 1 */

    while (ms-- != 0U)
    {
        while ((SysTick->CTRL & (1U << 16U)) == 0U)
        {
            /* Wait for COUNTFLAG */
        }
    }
    SysTick->CTRL = 0U;
}

/* --- GPIO Initialization --- */
void GPIO_Init(void)
{
    /* Enable Clocks for GPIOA, GPIOB, GPIOC */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN;

    /* PA0, PA1, PA4, PA5: Analog Mode (11b) */
    GPIOA->MODER |= (3U << 0U) | (3U << 2U) | (3U << 8U) | (3U << 10U);

    /* PC13: Onboard LED (Output, Push-pull, initially HIGH / OFF) */
    GPIOC->MODER &= ~(3U << (LED_PIN * 2U));
    GPIOC->MODER |=  (1U << (LED_PIN * 2U));
    GPIOC->BSRR = (1U << LED_PIN);

    /* PB5 & PB12: Inputs with Pull-ups (00b mode, 01b pupd) */
    GPIOB->MODER &= ~((3U << (BUTTON_PIN * 2U)) | (3U << (EMERGENCY_STOP_PIN * 2U)));
    GPIOB->PUPDR &= ~((3U << (BUTTON_PIN * 2U)) | (3U << (EMERGENCY_STOP_PIN * 2U)));
    GPIOB->PUPDR |=  ((1U << (BUTTON_PIN * 2U)) | (1U << (EMERGENCY_STOP_PIN * 2U)));

    /* PB9: Buzzer Output (Active-LOW, initially HIGH / Silent) */
    GPIOB->MODER &= ~(3U << (BUZZER_PIN * 2U));
    GPIOB->MODER |=  (1U << (BUZZER_PIN * 2U));
    GPIOB->BSRR = (1U << BUZZER_PIN);

    /* PB0, PB1, PB2: External LEDs (Output, initially LOW / OFF) */
    GPIOB->MODER &= ~((3U << (RED_LED_PIN * 2U)) |
                      (3U << (GREEN_LED_PIN * 2U)) |
                      (3U << (YELLOW_LED_PIN * 2U)));
    GPIOB->MODER |=  ((1U << (RED_LED_PIN * 2U)) |
                      (1U << (GREEN_LED_PIN * 2U)) |
                      (1U << (YELLOW_LED_PIN * 2U)));
    GPIOB->BSRR = (1U << (RED_LED_PIN + 16U)) |
                  (1U << (GREEN_LED_PIN + 16U)) |
                  (1U << (YELLOW_LED_PIN + 16U));

    /* PA8 & PA9: Motor Direction (Output, initially de-energized) */
    GPIOA->MODER &= ~((3U << (MOTOR_IN3 * 2U)) | (3U << (MOTOR_IN4 * 2U)));
    GPIOA->MODER |=  ((1U << (MOTOR_IN3 * 2U)) | (1U << (MOTOR_IN4 * 2U)));
    GPIOA->BSRR = (1U << (MOTOR_IN3 + 16U)) | (1U << (MOTOR_IN4 + 16U));

    /* PA6: TIM3_CH1 AF2 PWM */
    GPIOA->MODER &= ~(3U << (MOTOR_ENB_PIN * 2U));
    GPIOA->MODER |=  (2U << (MOTOR_ENB_PIN * 2U));
    GPIOA->AFR[0] &= ~(0xFU << (MOTOR_ENB_PIN * 4U));
    GPIOA->AFR[0] |=  (2U << (MOTOR_ENB_PIN * 4U));
}

/* --- USART2 Telemetry (PA2 TX, 9600 Baud, 8-N-1) --- */
void USART2_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 & PA3: AF7 */
    GPIOA->MODER &= ~((3U << 4U) | (3U << 6U));
    GPIOA->MODER |=  ((2U << 4U) | (2U << 6U));
    GPIOA->AFR[0] &= ~((0xFU << 8U) | (0xFU << 12U));
    GPIOA->AFR[0] |=  ((7U << 8U) | (7U << 12U));

    USART2->CR1 = 0U;
    USART2->CR2 = 0U;
    USART2->CR3 = 0U;
    /* 16 MHz / (16 * 9600) = 104.166 => Mantissa 104, Fraction 3 (0x683 = 1667) */
    USART2->BRR = 1667U;
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void USART2_SendChar(char c)
{
    uint32_t timeout = 100000U;
    while ((USART2->SR & USART_SR_TXE) == 0U)
    {
        if (timeout-- == 0U) return;
    }
    USART2->DR = (uint8_t)c;
}

static void USART2_SendString(const char *s)
{
    while (*s != '\0')
    {
        USART2_SendChar(*s++);
    }
}

void USART2_SendTelemetry(void)
{
    char line[128];
    const char *dirStr = (emergencyStopLatched) ? "STOPPED" :
                         (motorDirection == MOTOR_FORWARD) ? "FORWARD" :
                         (motorDirection == MOTOR_REVERSE) ? "REVERSE" : "STOPPED";
    uint32_t alarm = ((potData >= POT_THRESHOLD) || (temperatureC > TEMP_THRESHOLD)) ? 1U : 0U;

    int len = snprintf(line, sizeof(line),
        "%lu,%lu,%lu,%lu,%lu,%lu,%s,%lu,%lu\r\n",
        (unsigned long)potData,
        (unsigned long)temperatureC,
        (unsigned long)batteryVoltage_mV,
        (unsigned long)currentADC,
        (unsigned long)motorSpeedPercent,
        (unsigned long)motorPWM,
        dirStr,
        (unsigned long)alarm,
        (unsigned long)emergencyStopLatched);

    if (len > 0 && len < (int)sizeof(line))
    {
        USART2_SendString(line);
    }
}

/* --- ADC Initialization & Channel Reading --- */
void ADC_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    ADC->CCR &= ~(3U << 16U); /* PCLK2 / 2 */

    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;

    /* 84 cycle sample time on channels 0, 1, 4, 5 */
    ADC1->SMPR2 &= ~((7U << 0U) | (7U << 3U) | (7U << 12U) | (7U << 15U));
    ADC1->SMPR2 |=  ((4U << 0U) | (4U << 3U) | (4U << 12U) | (4U << 15U));

    ADC1->SQR1 = 0U; /* 1 conversion */
    ADC1->CR2 |= ADC_CR2_ADON;
    delay_ms(2U);
}

uint32_t ADC_Read(uint32_t channel)
{
    uint32_t timeout = 100000U;

    ADC1->SQR3 = channel & 0x1FU;
    ADC1->SR &= ~ADC_SR_EOC;
    ADC1->CR2 |= ADC_CR2_SWSTART;

    while ((ADC1->SR & ADC_SR_EOC) == 0U)
    {
        Emergency_Stop_Check();
        if (emergencyStopLatched) return 0U;
        if (timeout-- == 0U) return 0U;
    }
    return (ADC1->DR & ADC_MAX_VALUE);
}

/* --- TIM3 PWM Motor Drive --- */
void Motor_PWM_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->CR1 = 0U;
    TIM3->PSC = 15U;                /* 16 MHz / 16 = 1 MHz count clock */
    TIM3->ARR = MOTOR_PWM_MAX - 1U; /* 1 kHz PWM */

    TIM3->CCMR1 &= ~(7U << 4U);
    TIM3->CCMR1 |=  (6U << 4U) | (1U << 3U); /* PWM mode 1 + Preload */
    TIM3->CCER |= TIM_CCER_CC1E;
    TIM3->CCR1 = 0U;
    TIM3->EGR |= TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

void Motor_Stop(void)
{
    TIM3->CCR1 = 0U;
    TIM3->EGR |= TIM_EGR_UG;
    GPIOA->BSRR = (1U << (MOTOR_IN3 + 16U)) | (1U << (MOTOR_IN4 + 16U));
    motorPWM = 0U;
    motorSpeedPercent = 0U;
    motorDirection = MOTOR_STOPPED;
}

void Motor_SetSpeed(uint32_t adcValue)
{
    if (emergencyStopLatched != 0U)
    {
        Motor_Stop();
        return;
    }

    if (adcValue < POT_THRESHOLD)
    {
        /* Forward Mode */
        GPIOA->BSRR = (1U << MOTOR_IN3) | (1U << (MOTOR_IN4 + 16U));
        motorDirection = MOTOR_FORWARD;
        motorPWM = MOTOR_FORWARD_PWM;
        motorSpeedPercent = 70U;
    }
    else
    {
        /* Reverse / Maximum Load Mode */
        GPIOA->BSRR = (1U << (MOTOR_IN3 + 16U)) | (1U << MOTOR_IN4);
        motorDirection = MOTOR_REVERSE;
        motorPWM = MOTOR_REVERSE_PWM;
        motorSpeedPercent = 100U;
    }
    TIM3->CCR1 = motorPWM;
}

/* --- I2C1 Master Driver (PB6 SCL, PB7 SDA) --- */
void I2C1_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB->MODER &= ~((3U << 12U) | (3U << 14U));
    GPIOB->MODER |=  ((2U << 12U) | (2U << 14U)); /* AF4 */
    GPIOB->OTYPER |= (1U << 6U) | (1U << 7U);      /* Open-drain */
    GPIOB->PUPDR &= ~((3U << 12U) | (3U << 14U));  /* Pull-up */
    GPIOB->PUPDR |=  ((1U << 12U) | (1U << 14U));
    GPIOB->AFR[0] |= (4U << 24U) | (4U << 28U);

    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0U;
    I2C1->CR2 = 16U; /* 16 MHz APB1 */
    I2C1->CCR = 80U; /* 100 kHz standard mode */
    I2C1->TRISE = 17U;
    I2C1->CR1 |= I2C_CR1_PE;
}

static uint8_t I2C1_WriteReg(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint32_t t = 50000U;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t);
    I2C1->CR1 |= I2C_CR1_START;
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_SB) && --t);
    (void)I2C1->SR1;
    I2C1->DR = (uint8_t)(addr << 1U);
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_ADDR) && --t);
    (void)I2C1->SR1; (void)I2C1->SR2;
    I2C1->DR = reg;
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_TXE) && --t);
    I2C1->DR = val;
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_BTF) && --t);
    I2C1->CR1 |= I2C_CR1_STOP;
    return 1U;
}

static uint8_t I2C1_ReadReg(uint8_t addr, uint8_t reg, uint8_t *val)
{
    uint32_t t = 50000U;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t);
    I2C1->CR1 |= I2C_CR1_START;
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_SB) && --t);
    (void)I2C1->SR1;
    I2C1->DR = (uint8_t)(addr << 1U);
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_ADDR) && --t);
    (void)I2C1->SR1; (void)I2C1->SR2;
    I2C1->DR = reg;
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_BTF) && --t);

    I2C1->CR1 |= I2C_CR1_START; /* Repeated START */
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_SB) && --t);
    (void)I2C1->SR1;
    I2C1->DR = (uint8_t)((addr << 1U) | 1U);
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_ADDR) && --t);
    I2C1->CR1 &= ~I2C_CR1_ACK;
    (void)I2C1->SR1; (void)I2C1->SR2;
    I2C1->CR1 |= I2C_CR1_STOP;
    t = 50000U;
    while (!(I2C1->SR1 & I2C_SR1_RXNE) && --t);
    *val = (uint8_t)I2C1->DR;
    I2C1->CR1 |= I2C_CR1_ACK;
    return 1U;
}

/* --- SSD1306 OLED Minimal Primitives --- */
static void OLED_Command(uint8_t cmd) { I2C1_WriteReg(OLED_ADDR, 0x00U, cmd); }
static void OLED_Data(uint8_t data)   { I2C1_WriteReg(OLED_ADDR, 0x40U, data); }

void OLED_Init(void)
{
    delay_ms(50U);
    OLED_Command(0xAEU); /* Display OFF */
    OLED_Command(0xD5U); OLED_Command(0x80U);
    OLED_Command(0xA8U); OLED_Command(0x3FU);
    OLED_Command(0xD3U); OLED_Command(0x00U);
    OLED_Command(0x40U);
    OLED_Command(0x8DU); OLED_Command(0x14U); /* Charge pump ON */
    OLED_Command(0x20U); OLED_Command(0x00U);
    OLED_Command(0xA1U); OLED_Command(0xC8U);
    OLED_Command(0xDAU); OLED_Command(0x12U);
    OLED_Command(0x81U); OLED_Command(0x7FU);
    OLED_Command(0xA4U); OLED_Command(0xA6U);
    OLED_Command(0xAFU); /* Display ON */
    OLED_Clear();
}

void OLED_Clear(void)
{
    for (uint8_t p = 0; p < 8; p++)
    {
        OLED_Command(0xB0U | p);
        OLED_Command(0x00U);
        OLED_Command(0x10U);
        for (uint8_t c = 0; c < 128; c++) OLED_Data(0x00U);
    }
}

void OLED_DisplaySensors(void)
{
    /* Refreshes display page headers */
    OLED_Command(0xB0U); OLED_Command(0x00U); OLED_Command(0x10U);
}

void OLED_DisplayEmergencyStop(void)
{
    OLED_Clear();
}

/* --- MPU6050 Vibration Monitoring --- */
void MPU6050_Init(void)
{
    uint8_t id = 0U;
    mpu6050Connected = 0U;
    if (I2C1_ReadReg(MPU6050_ADDR, MPU6050_REG_WHO_AM_I, &id) && id == 0x68U)
    {
        I2C1_WriteReg(MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, 0x00U); /* Wake up */
        delay_ms(50U);
        I2C1_WriteReg(MPU6050_ADDR, MPU6050_REG_SMPLRT_DIV, 0x07U);
        I2C1_WriteReg(MPU6050_ADDR, MPU6050_REG_CONFIG, 0x06U);
        I2C1_WriteReg(MPU6050_ADDR, MPU6050_REG_ACCEL_CONFIG, 0x00U); /* +/- 2g */
        mpu6050Connected = 1U;
    }
}

void MPU6050_UpdateVibration(void)
{
    uint8_t hi, lo;
    if (!mpu6050Connected) return;

    I2C1_ReadReg(MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, &hi);
    I2C1_ReadReg(MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H + 1, &lo);
    accelX = (int16_t)(((uint16_t)hi << 8) | lo);

    if (!vibrationInitialized)
    {
        previousAccelX = accelX;
        vibrationInitialized = 1U;
        return;
    }

    int32_t dx = (int32_t)accelX - previousAccelX;
    if (dx < 0) dx = -dx;
    vibrationValue = (uint32_t)dx;
    vibrationAlarm = (vibrationValue >= VIBRATION_THRESHOLD) ? 1U : 0U;
    previousAccelX = accelX;
}

/* --- System Reading & Supervisory Control --- */
void Read_Sensors(void)
{
    if (emergencyStopLatched) return;

    potData = ADC_Read(0U);
    tempData = ADC_Read(1U);
    batteryADC = ADC_Read(4U);
    currentADC = ADC_Read(5U);

    voltage_mV = (tempData * ADC_REFERENCE_MV) / ADC_MAX_VALUE;
    temperatureC = voltage_mV / 10U;
    batteryVoltage_mV = (batteryADC * 6600U) / ADC_MAX_VALUE;

    buttonState = ((GPIOB->IDR & (1U << BUTTON_PIN)) == 0U) ? 1U : 0U;
    sensorAlarm = ((potData >= POT_THRESHOLD) || (temperatureC > TEMP_THRESHOLD)) ? 1U : 0U;
}

void External_LED_Control(void)
{
    if (emergencyStopLatched)
    {
        GPIOB->BSRR = (1U << (GREEN_LED_PIN + 16U)) | (1U << (YELLOW_LED_PIN + 16U));
        return;
    }

    GPIOB->BSRR = (1U << (RED_LED_PIN + 16U));
    if (sensorAlarm || vibrationAlarm)
    {
        GPIOB->BSRR = (1U << YELLOW_LED_PIN) | (1U << (GREEN_LED_PIN + 16U));
    }
    else
    {
        GPIOB->BSRR = (1U << GREEN_LED_PIN) | (1U << (YELLOW_LED_PIN + 16U));
    }
}

void LED_Control(void)
{
    if (emergencyStopLatched) return;
    if (sensorAlarm)
    {
        GPIOC->ODR ^= (1U << LED_PIN);
    }
    else
    {
        GPIOC->BSRR = (1U << LED_PIN);
    }
}

void Buzzer_Control(void)
{
    if (emergencyStopLatched)
    {
        GPIOB->BSRR = (1U << BUZZER_PIN); /* Silent */
        return;
    }

    if ((potData > POT_THRESHOLD) || vibrationAlarm)
    {
        buzzerToggleState ^= 1U;
        if (buzzerToggleState)
        {
            GPIOB->BSRR = (1U << (BUZZER_PIN + 16U)); /* Sound */
        }
        else
        {
            GPIOB->BSRR = (1U << BUZZER_PIN);        /* Silent */
        }
    }
    else
    {
        GPIOB->BSRR = (1U << BUZZER_PIN);            /* Silent */
        buzzerToggleState = 0U;
    }
}

void Emergency_Stop_Check(void)
{
    if ((GPIOB->IDR & (1U << EMERGENCY_STOP_PIN)) == 0U)
    {
        emergencyStopPressed = 1U;
        emergencyStopLatched = 1U;
    }
}

void Emergency_Stop_Execute(void)
{
    Motor_Stop();
    GPIOB->BSRR = (1U << BUZZER_PIN); /* Silence buzzer */
    GPIOB->BSRR = (1U << (GREEN_LED_PIN + 16U)) | (1U << (YELLOW_LED_PIN + 16U));

    potData = 0U;
    tempData = 0U;
    batteryADC = 0U;
    currentADC = 0U;
    voltage_mV = 0U;
    temperatureC = 0U;
    batteryVoltage_mV = 0U;
    sensorAlarm = 0U;

    OLED_DisplayEmergencyStop();

    /* Permanent Lockout: Rapid 5 Hz Toggle on PC13 and Red LED */
    while (1)
    {
        GPIOB->ODR ^= (1U << RED_LED_PIN);
        GPIOC->ODR ^= (1U << LED_PIN);
        USART2_SendTelemetry();
        delay_ms(100U);
    }
}

/* --- Main Application --- */
int main(void)
{
    GPIO_Init();
    USART2_Init();
    ADC_Init();
    I2C1_Init();
    OLED_Init();
    Motor_PWM_Init();
    MPU6050_Init();

    Motor_Stop();
    GPIOB->BSRR = (1U << BUZZER_PIN);
    GPIOC->BSRR = (1U << LED_PIN);

    Emergency_Stop_Check();
    if (emergencyStopLatched) Emergency_Stop_Execute();

    while (1)
    {
        Emergency_Stop_Check();
        if (emergencyStopLatched) Emergency_Stop_Execute();

        Read_Sensors();

        Emergency_Stop_Check();
        if (emergencyStopLatched) Emergency_Stop_Execute();

        MPU6050_UpdateVibration();
        Motor_SetSpeed(potData);

        External_LED_Control();
        LED_Control();
        Buzzer_Control();
        OLED_DisplaySensors();

        USART2_SendTelemetry();
        delay_ms(200U);
    }
}
