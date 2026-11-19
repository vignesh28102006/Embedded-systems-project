#include "stm32f4xx.h"

int main(void){

    RCC->AHB1ENR |= (1U<<0);
    RCC->AHB1ENR |= (1U<<1);
    RCC->AHB1ENR |= (1U<<2);

    GPIOC->MODER &= ~(3U<<28);
    GPIOC->MODER |= (1U<<28);

    SysTick->LOAD = 16000000-1
    SysTick->VAL = 0;
    Systick->CTRL = 5;

    while(1){
        if(SysTick->CTRL & (1U<<16)){
            GPIOC->ODR ^= (1U<<14);
        }
    }
}