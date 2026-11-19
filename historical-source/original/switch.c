// #include "stm32f4xx.h"
// int main (void) {
//     RCC->AHB1ENR |= (1U<<0);
//     RCC->AHB1ENR |= (1U<<2);

//     GPIOA->MODER &= ~(3U<<0);
//     GPIOA->PUPDR &= ~(3U<<0);
//     GPIOA->PUPDR |= (1U<<0);

//     GPIOC->MODER &= ~(3U<<26);
//     GPIOC->MODER |= (1U<<26);

//     while(1){
//         if(GPIOA->IDR & (1U<<0)){

//             while (1){
//                 GPIOC->ODR &= ~(1U<<13);
                
//                 for(int i=0;i<100000;i++){}

//                 GPIOC->ODR |= (1U<<13);

//                 for(int i=0;i<100000;i++){}
//             }
//         }else{
//             GPIOC->ODR |= (1U<<13)
//         }
//     }

// }

// #include "stm32f4xx.h"

// void delay(int x){
//     for(int i=0;i<x;i++){}
// }

// int main (void){
//     RCC->AHB1ENR |= (1U<<0);
//     RCC->AHB1ENR |= (1U<<1);
//     RCC->AHB1ENR |= (1U<<2);

//     GPIOB->MODER &= ~(3U<<20);
//     GPIOB->PUPDR &= ~(3U<<20);
//     GPIOB->PUPDR |= (1U<<20);

//     GPIOA->MODER &= ~(3U<<0):
//     GPIOA->PUPDR &= ~(3U<<0);
//     GPIOA->PUPDR |= (1U<<0);
    
//     GPIOA->MODER &== ~(3U<<12):
//     GPIOA->PUPDR &= ~(3U<<12);
//     GPIOA->PUPDR |= (1U<<12);

//     GPIOC->MODER &= ~(3U<<26);
//     GPIOC->MODER |= (1U<<26);

//     GPIOC->MODER &= ~(3U<<28);
//     GPIOC->MODER |= (1U<<28);

//     GPIOC->MODER &= ~(3U<<18);
//     GPIOC->MODER |= (1U<<18);

//     while(1) {

//         if(!(GPIOB->IDR & (1U<<10) ) && !(GPIOA->IDR & (1U<<0)) ){
//             while (1) {
//                 GPIOC->ODR &= ~(1U<<13);
//                 for(int i=0;i<1000000;i++){}

//                 GPIOC->ODR |= (1U<<13);
//                 for(int i=0;i<1000000;i++){}

//                 GPIOC->ODR |= (1U<<14);
//                 for(int i=0;i<1000000;i++){}

//                 GPIOC->ODR &= ~(1U<<14);
//                 for(int i=0;i<1000000;i++){}

//                 GPIOB->ODR |= (1U<<9);
//                 for(int i=0;i<1000000;i++){}

//                 GPIOB->ODR &= ~(1U<<9);
//                 for(int i=0;i<1000000;i++){}
//             }
//         }else if(!(GPIOA->IDR & (1U<<6) ) && !(GPIOA->ID & (1U<<0))){
//             while (1) {

//                 GPIOB->ODR |= (1U<<9);
//                 delay();

//                 GPIOB->ODR &= ~(1U<<9);
//                 for(int i=0;i<1000000;i++){}

//                 GPIOC->ODR |= (1U<<14);
//                 for(int i=0;i<1000000;i++){}

//                 GPIOC->ODR &= ~(1U<<14);
//                 for(int i=0;i<1000000;i++){}

//                 GPIOC->ODR &= ~(1U<<13);
//                 for(int i=0;i<1000000;i++){}

//                 GPIOC->ODR |= (1U<<13);
//                 for(int i=0;i<1000000;i++){}
                
//             }
//         }
//     }


// }


#include "stm32f4xx.h"

void delay (int x){
    for(int i = 0; i < x ;i++){}
}

int main(void){
    RCC->AHB1ENR(1U<<0);
    RCC->AHB1ENR(1U<<1);
    RCC->AHB1ENR(1U<<2);

    GPIOA->MODER &= ~(3U<<12);
    GPIOA->PUPDR &= ~(3U<<12);
    GPIOA->PUPDR |= (1U<<12);

    GPIOB->MODER &= ~(3U<<20);
    GPIOB->PUPDR &= ~(3U<<20);
    GPIOB->PUPDR |= (1U<<20);

    GPIOC->MODER &= ~(3U<<28);
    GPIOC->MODER |= (1U<<28);

    GPIOB->MODER &= ~(3U<<18);
    GPIOB->MODER |= (1U<<18);

    GPIOC->MODER &= ~(3U<<26);
    GPIOC->MODER |= (1U<<26);

    while(1){
        if(!(GPIOB->IDR & (1U<<0))){

            GPIOC->ODR |= (1U<<14);
            delay(1000000);
            GPIOC->ODR &= ~(1U<<14);
            delay(1000000);
        }
    }