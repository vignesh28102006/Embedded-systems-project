// #include "stm32f4xx.h"
// int main(void){
//     RCC->AHB1ENR |= (1U << 0);
//     RCC->AHB1ENR |= (1U << 1);
//     RCC->AHB1ENR |= (1U << 2);
//     RCC->APB2ENR |= (1U << 14);

//     GPIOC->MODER &= ~(3U << 28);
//     GPIOC->MODER |= (1U << 28);

//     GPIOA->MODER &= ~(3U << 12);
//     GPIOA->PUPDR &= ~(3U << 12);
//     GPIOA->PUPDR |= (1U << 12);

//     SYSCFG->EXTICR[1] &= ~(15U << 8);
//     SYSCFG->EXTICR[1] |= (0U << 8);

//     EXTI->IMR |= (1U << 6);

//     EXTI->FTSR |= (1U << 6);

//     NVIC_EnableIRQ(EXTI9_5_IRQn);
//     while(1){

//     }
// }
// void EXTI9_5_IRQHandler(void){
//     if(EXTI->PR & (1U << 6)){
//         EXTI->PR |= (1U << 6);
//         GPIOC->ODR ^= (1U << 14);
//     }
// }

#include "stm32f4xx.h"
#include <stdio.h>

#define PCF8574_ADDR 0x4E

volatile int adcData;
volatile int active = 0;
volatile int lcdFlag = 0;
volatile int count = 0;

const int threshold = 2048;

// Delay function
void delay_ms(uint32_t ms)
{
    for(uint32_t i = 0; i < ms * 8000; i++)
    {
        __NOP();
    }
}

// I2C initialization
void I2C1_Init(void)
{
    RCC->AHB1ENR |= (1U << 1);
    RCC->APB1ENR |= (1U << 21);

    GPIOB->MODER &= ~((3U << 12) | (3U << 14));
    GPIOB->MODER |= ((2U << 12) | (2U << 14));

    GPIOB->OTYPER |= (1U << 6) | (1U << 7);
    GPIOB->OSPEEDR |= (3U << 12) | (3U << 14);

    GPIOB->PUPDR &= ~((3U << 12) | (3U << 14));
    GPIOB->PUPDR |= ((1U << 12) | (1U << 14));

    GPIOB->AFR[0] &= ~((15U << 24) | (15U << 28));
    GPIOB->AFR[0] |= ((4U << 24) | (4U << 28));

    I2C1->CR1 |= (1U << 15);
    I2C1->CR1 &= ~(1U << 15);

    I2C1->CR2 = 16;
    I2C1->CCR = 80;
    I2C1->TRISE = 17;

    I2C1->CR1 |= (1U << 0);
}

// I2C write
void I2C1_Write(uint8_t addr, uint8_t data)
{
    while(I2C1->SR2 & (1U << 1));

    I2C1->CR1 |= (1U << 8);
    while(!(I2C1->SR1 & (1U << 0)));

    I2C1->DR = addr;
    while(!(I2C1->SR1 & (1U << 1)));

    (void)I2C1->SR2;

    while(!(I2C1->SR1 & (1U << 7)));
    I2C1->DR = data;

    while(!(I2C1->SR1 & (1U << 2)));

    I2C1->CR1 |= (1U << 9);
}

// LCD command
void LCD_SendCmd(uint8_t cmd)
{
    uint8_t high = cmd & 0xF0;
    uint8_t low = (cmd << 4) & 0xF0;

    I2C1_Write(PCF8574_ADDR, high | 0x0C);
    delay_ms(1);
    I2C1_Write(PCF8574_ADDR, high | 0x08);

    I2C1_Write(PCF8574_ADDR, low | 0x0C);
    delay_ms(1);
    I2C1_Write(PCF8574_ADDR, low | 0x08);
}

// LCD data
void LCD_SendData(uint8_t data)
{
    uint8_t high = data & 0xF0;
    uint8_t low = (data << 4) & 0xF0;

    I2C1_Write(PCF8574_ADDR, high | 0x0D);
    delay_ms(1);
    I2C1_Write(PCF8574_ADDR, high | 0x09);

    I2C1_Write(PCF8574_ADDR, low | 0x0D);
    delay_ms(1);
    I2C1_Write(PCF8574_ADDR, low | 0x09);
}

// LCD initialization
void LCD_Init(void)
{
    delay_ms(50);

    LCD_SendCmd(0x33);
    LCD_SendCmd(0x32);
    LCD_SendCmd(0x28);
    LCD_SendCmd(0x0C);
    LCD_SendCmd(0x06);
    LCD_SendCmd(0x01);

    delay_ms(5);
}

// Set LCD cursor
void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0 ? 0x80 : 0xC0) + col;
    LCD_SendCmd(addr);
}

// Print on LCD
void LCD_Print(char *str)
{
    while(*str)
    {
        LCD_SendData(*str++);
    }
}

// ADC initialization
void adc_Init(void)
{
    RCC->AHB1ENR |= (1U << 0);
    RCC->APB2ENR |= (1U << 8);

    GPIOA->MODER &= ~(3U << 0);
    GPIOA->MODER |= (3U << 0);

    ADC1->SMPR2 = 0;
    ADC->CCR = 0;

    ADC1->CR1 = 0;
    ADC1->CR2 = 0;

    ADC1->CR2 |= (1U << 1);

    ADC1->SQR3 = 0;
    ADC1->SQR1 = 0;

    ADC1->CR1 |= (1U << 5);

    NVIC_EnableIRQ(ADC_IRQn);

    ADC1->CR2 |= (1U << 0);

    ADC1->CR2 |= (1U << 30);
}

// SysTick initialization
void Sys_Init(void)
{
    SysTick->LOAD = 16000000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = 7;
}

// External interrupt initialization: PB2
void inter_Init(void)
{
    RCC->AHB1ENR |= (1U << 1);
    RCC->APB2ENR |= (1U << 14);

    GPIOB->MODER &= ~(3U << 4);

    GPIOB->PUPDR &= ~(3U << 4);
    GPIOB->PUPDR |= (1U << 4);

    SYSCFG->EXTICR[0] &= ~(15U << 8);
    SYSCFG->EXTICR[0] |= (1U << 8);

    EXTI->IMR |= (1U << 2);
    EXTI->FTSR |= (1U << 2);

    EXTI->PR = (1U << 2);

    NVIC_EnableIRQ(EXTI2_IRQn);
}

// Main program
int main(void)
{
    char buf[16];

    I2C1_Init();
    LCD_Init();

    RCC->AHB1ENR |= (1U << 2);

    GPIOC->MODER &= ~(3U << 28);
    GPIOC->MODER |= (1U << 28);

    adc_Init();
    Sys_Init();
    inter_Init();

    while(1)
    {
        if(lcdFlag)
        {
            lcdFlag = 0;

            LCD_SendCmd(0x01);

            if(active)
            {
                LCD_SetCursor(0, 0);
                LCD_Print("POT Value:");

                LCD_SetCursor(1, 0);
                sprintf(buf, "%d", adcData);
                LCD_Print(buf);
            }
            else
            {
                LCD_SetCursor(0, 0);
                LCD_Print("Monitoring OFF");
            }

            delay_ms(500);
        }
    }
}

// ADC interrupt
void ADC_IRQHandler(void)
{
    if(ADC1->SR & (1U << 1))
    {
        adcData = ADC1->DR;

        if(active)
        {
            lcdFlag = 1;

            if(adcData >= threshold)
            {
                GPIOC->ODR |= (1U << 14);
            }
        }
        else
        {
            GPIOC->ODR &= ~(1U << 14);
        }
    }
}

// SysTick interrupt
void SysTick_Handler(void)
{
    if(active)
    {
        if(adcData < threshold)
        {
            GPIOC->ODR ^= (1U << 14);
        }
        else
        {
            GPIOC->ODR |= (1U << 14);
        }
    }

    count++;
}

// External interrupt
void EXTI2_IRQHandler(void)
{
    if(EXTI->PR & (1U << 2))
    {
        EXTI->PR = (1U << 2);

        active ^= 1;

        if(!active)
        {
            GPIOC->ODR &= ~(1U << 14);
        }

        lcdFlag = 1;
    }
}