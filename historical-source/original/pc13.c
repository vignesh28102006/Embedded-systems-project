#include "stm32f4xx.h"
int main (void){
    RCC->AHB1ENR |= (1<<2);
    GPIOC->MODER &= ~(3U<<26);
    GPIOC->MODER |= (1U<<26);
    GPIOC->ODR &= ~(1U<<13);
}