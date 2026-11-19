#include "stm32f4xx.h"

int adcData;

int main(void){

    RCC->AHB1ENR |= (1U<<0);
    RCC->AHB1ENR |= (1U<<1);
    RCC->AHB1ENR |= (1U<<2);

    RCC->APB2ENR |= (1U << 8);

    GPIOA->MODER &= ~(3U<<12);
    GPIOA->PUPDR &= ~(3U<<12);
    GPIOA->PUPDR |= (1U<<12);

    GPIOB->MODER &= ~(3U<<20);
    GPIOB->PUPDR &= ~(3U<<20);
    GPIOB->PUPDR |= (1U<<20);

    GPIOC->MODER &= ~(3U<<26);
    GPIOC->MODER |= (1U<<26);

    GPIOB->MODER &= ~(3U<<18);
    GPIOB->MODER |= (1U<<18);

    GPIOC->MODER &= ~(3U<<28);
    GPIOC->MODER |= (1U<<28);

    GPIOA->MODER |= (3U<<0);

    ADC1->SMPR2 = 0;

    ADC->CCR = 0;
    ADC1->CCR1 = 0;
    ADC1->CCR2 = 0;

    ADC1->SQR3 = 0;
    ADC1->SQR1 = 0;

    ADC1->CR2 |= 1U;

    while((ADC1->CR2 = (1U<<1)) == 0){}

    ADC1->CR2 |= (1U<<30);

    while(1){
        while(!(ADC->SR & (1U<<1))){}

        abcData = ADC->DR ;

        if(!(GPIOA->IDR & (1U<<6))){

            GPIOC->ODR &= ~(1U<<13);

            if(!(GPIOB->IDR & (1U<<10))){
            GPIOB->ODR |= (1U<<9);
            GPIOC->ODR |= (1U<<14);
            }
            else{
                
                GPIOC->ODR &= ~(1U<<14);

                if(adcData<1000){
                GPIOB->ODR |= (1U<<9);

                for(int i=0;i<1000000;i++){}

                GPIOB->ODR &= ~(1U<<9);

                for(int i=0;i<1000000;i++){}
                
                }

                if(adcData>1000){
                    GPIOB->ODR |= (1U<<9);

                    for(int i=0;i<100000;i++){}

                    GPIOB->ODR &= ~(1U<<9);

                    for(int i=0;i<100000;i++){}
                }
            }

            

        }
        else{
            GPIOC->ODR |= (1U<<13);
            GPIOB->ODR &= ~(1U<<9);
            GPIOC->ODR &= ~(1U<<14);
        }

        


        
    }



    
}