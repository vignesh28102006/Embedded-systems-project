#include "stm32f4xx.h"

int d;

int main(void){

    RCC->AHB1ENR |= (1U<<0);
    RCC->AHB1ENR |= (1U<<1);
    RCC->AHB1ENR |= (1U<<2);

    RCC->APB2ENR |= (1U<<8);

    GPIOA->MODER |= (1U<<26);
    GPIOA->MODER |= (1U<<28);
    GPIOA->MODER |= (1U<<18);

    GPIOA->MODER |= (3U<<2);

    GPIOB->MODER &= ~(3U<<20);
    GPIOB->PUPDR &= ~(3U<<20);
    GPIOB->PUPDR |= (1U<<20);

    GPIOA->MODER &= ~(3U<<0);
    GPIOA->PUPDR &= ~(3U<<0);
    GPIOA->PUPDR |= (1U<<0);

    ADC1->SMPR2 = 0;
    ADC->CCR = 0;
    ADC1->CCR1 = 0;
    ADC1->CCR2 = 0;

    ADC1->SQR3 = 1;
    ADC1->SQR1 = 0;

    ADC1->CR2 |= 1U;

    while((ADC1->CR2 = 1U) == 0){}

    ADC1->CR2 |= (1U<<30);

    while(1){

        while(!(ADC1->CR2 & (1U<<1))){}

        d = ADC1->DR;

        if(!(GPIOA->IDR & (1U<<0))){
            GPIOC->ODR &= ~(1U<<13);
        }
        GPIOC->ODR |= (1U<<13);

        if(d<800){
            GPIOB->ODR |= (1U<<9);
        }
        if(d>800){
            GPIOB->ODR &= ~(1U<<9);
        }

        if(!(GPIOB->IDR & (1U<<10))){
            GPIOB->ODR &= ~(1U<<9);
        }

    }
}