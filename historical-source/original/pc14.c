#include "stm32fxx.h"
int main (void) {
    RCC->AHB1ENR |= (1<<2);
    GPIOC->MODER &= ~(3U<<28);
    GPIOC->MODER |= (1U<<28);
    GPIOC->ODR |= (1U<<14);
    
}