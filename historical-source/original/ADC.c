#include "stm32f4xx.h"

int adcData;

int main(void)
{
    // 1. Enable GPIOA clock
    // GPIOA clock is controlled by bit 0 of AHB1ENR
    RCC->AHB1ENR |= (1U << 0);

    // 2. Enable ADC1 clock
    // ADC1 clock is controlled by bit 8 of APB2ENR
    RCC->APB2ENR |= (1U << 8);

    // 3. Set PA0 to Analog mode
    // PA0 uses MODER bits [1:0]
    // 11 = Analog mode
    GPIOA->MODER |= (3U << 0);

    // 4. Set sampling time to 3 ADC cycles
    // Channel 0 uses SMPR2 bits [2:0]
    // 000 = 3 cycles
    ADC1->SMPR2 = 0;

    // 5. Set ADC prescaler to PCLK2/2
    // CCR = 0 gives PCLK2/2
    ADC->CCR = 0;

    // 6. Reset ADC control registers
    ADC1->CR1 = 0;
    ADC1->CR2 = 0;

    // 7. Enable continuous conversion
    // CR2 bit 1 = CONT
    ADC1->CR2 |= (1U << 1);

    // 8. Select ADC Channel 0
    // SQR3 = 0 means Channel 0
    ADC1->SQR3 = 0;

    // 9. Set conversion sequence length to 1
    // SQR1 = 0 means 1 conversion
    ADC1->SQR1 = 0;

    // 10. Enable ADC
    // CR2 bit 0 = ADON
    ADC1->CR2 |= 1U;

    // 11. Wait until ADC is ready
    while ((ADC1->CR2 & 1U) == 0)
    {
    }

    // 12. Start ADC conversion
    // CR2 bit 30 = SWSTART
    ADC1->CR2 |= (1U << 30);

    // 13. Continuously read ADC value
    while (1)
    {
        // Wait until conversion is complete
        // SR bit 1 = EOC
        while (!(ADC1->SR & (1U << 1)))
        {
        }

        // Read converted ADC value
        adcData = ADC1->DR;
    }
}
