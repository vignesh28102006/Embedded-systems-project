#include "stm32fxx.h"
int main (void) {
    RCC->AHB1ENR |= (1<<1);
    GPIOB->MODER &= ~(3U<<18);
    GPIOB->MODER |= (1U<<18);

    while(1){
        GPIOB->ODR |= (1U<<9);
        for(int i=0;i<180000;i++){}

        GPIOB->ODR &= ~(1U<<9);
        for(int i=0;i<180000;i++){}
    }
}