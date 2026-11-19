#include "stm32f4xx.h"

int main(void){
    RCC->AHB1ENR |= (1U<<0);
    RCC->AHB1ENR |= (1U<<1);
    RCC->AHB1ENR |= (1U<<2);

    GPIOC->MODER &= ~(3U<<28);
    GPIOC->MODER |= (1U<<28);

    GPIOA->MODER &= ~(3U<<12);
    GPIOA->PUPDR &= ~(3U<<12);
    GPIOA->PUPDR |= (1U<<12);

    GPIOB->MODER &= ~(3U<<20);
    GPIOB->PUPDR &= ~(3U<<20);
    GPIOB->PUPDR |= (1U<<20);

    RCC->APB2ENR |= (1U<<14);

    SYSCFG->EXTICR[1] &= ~(15<<8);
    SYSCFG->EXTICR[1] |= (0<<8);

    SYSCFG->EXTICR[2] &= ~(15<<8);
    SYSCFG->EXTICR[2] |= (1<<8);

    EXTI->IMR |= (1<<6);
    EXTI->IMR |= (1<<10);

    EXTI->FTSR = (1<<6);
    EXTI->FTSR = (1<<10);

    NVIC_EnableIQR(EXTI9_5_IQRn);
    NVIC_EnavleIQR(EXTI15_10_IQRn);

    while(1){

    }

}

void EXTI9_5_IQRHandler(void){

    if(EXTI->PR & (1<<6)){

        EXTI->PR |= (1<<6);

        GPIOC->ODR |= (1U<<14);
    }
}

void EXTI15_10_IQRHandler(void){

    if(EXTI->PR & (1<<10)){

        EXTI->PR
    }
}
