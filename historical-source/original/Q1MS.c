#include "stm32f4xx.h"

int adcData ;

int main(void){
    RCC->AHB1ENR |= (1U<<0);
    RCC->AHB1ENR |= (1U<<2);
    RCC->APB2ENR |= (1U<<8);

    GPIOC->MODER |= (1U<<26);
    GPIOA->MODER |= (1U<<0);

    ADC1->SMPR2 = 0;
    ADC->CCR = 0;
    ADC1->CCR1 = 0;
    ADC1->CCR2 = 0;

    ADC1->SQR3 = 0;
    ADC1->SQR1 = 0;

    ADC1->CR2 |= 1U;

    while((ADC1->CR2 = 1U) == 0){
    }

    ADC1->CR2 |= (1U<<30);

    while(1){
        while(!(ADC1->CR2 & (1U<<1))){}

        adcData = ADC1->DR;

        if(adcData<=1023){
            GPIOC->ODR &= ~(1U<<13);
            for(int i=0;i<100000;i++){}

            GPIOC->ODR &= ~(1U<<13);
            for(int i=0;i<100000;i++){}

        }
        if(adcData>=1023 && adcData<=2047 ){

            for(int j=0;j<2;j++){
                GPIOC->ODR &= ~(1U<<13);
                for(int i=0;i<100000;i++){}

                GPIOC->ODR &= ~(1U<<13);
                for(int i=0;i<100000;i++){}
            }
            
            
        }
        if(adcData>=2048 && adcData<=3071 ){

            for(int j=0;j<3;j++){
                GPIOC->ODR &= ~(1U<<13);
                for(int i=0;i<100000;i++){}

                GPIOC->ODR &= ~(1U<<13);
                for(int i=0;i<100000;i++){}
            }

        }
        if(adcData>=3072 && adcData<=4095 ){

            for(int j=0;j<4;j++){
                GPIOC->ODR &= ~(1U<<13);
                for(int i=0;i<100000;i++){}

                GPIOC->ODR &= ~(1U<<13);
                for(int i=0;i<100000;i++){}
            }
            
        }

    }

}