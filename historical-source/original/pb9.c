#include "stm32fxx.h"
int main(void){
    RCC->AHB1ENR |= (1<<1);
    RCC->AHB1ENR |= (1<<2);
    GPIOB->MODER &= ~(1<<18);
    GPIOB->MODER |= (1<<18);
    GPIOB->ODR |= (1<<9);
    
}