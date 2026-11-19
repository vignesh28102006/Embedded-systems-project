#include "stm32f4xx.h"

int main(void)
{
    // =========================================================
    // 1. ENABLE GPIOC CLOCK
    // =========================================================
    //
    // RCC->AHB1ENR controls the clocks for GPIO ports.
    //
    // Bit 0 -> GPIOA
    // Bit 1 -> GPIOB
    // Bit 2 -> GPIOC
    //
    // We need GPIOC because the LED is connected to PC14.
    //
    // 1U << 2 means:
    // Take the number 1 and shift it left by 2 positions.
    //
    // 00000001
    //       ↓
    // 00000100
    //
    // Therefore bit 2 becomes 1.

    RCC->AHB1ENR |= (1U << 2);     // Enable GPIOC clock


    // =========================================================
    // 2. CONFIGURE PC14 AS OUTPUT
    // =========================================================
    //
    // GPIO pins are configured using the MODER register.
    //
    // Each GPIO pin uses 2 bits in MODER.
    //
    // For PC14:
    //
    // Pin number = 14
    // Bit position = 14 × 2 = 28
    //
    // Therefore PC14 uses MODER bits 28 and 29.
    //
    // MODER configuration:
    //
    // 00 = Input
    // 01 = Output
    // 10 = Alternate Function
    // 11 = Analog
    //
    // We want:
    //
    // PC14 = Output
    // Therefore the two bits must be 01.


    // First clear bits 28 and 29.
    //
    // 3 in binary is:
    //
    // 11
    //
    // So 3 << 28 selects the two bits belonging to PC14.

    GPIOC->MODER &= ~(3U << 28);


    // Now set the bits to 01.
    //
    // 1 in binary = 01
    //
    // Therefore PC14 becomes Output mode.

    GPIOC->MODER |= (1U << 28);


    // =========================================================
    // 3. CONFIGURE SYSTICK TIMER
    // =========================================================
    //
    // We want SysTick to generate an interrupt every 1 second.
    //
    // Assume processor clock = 16 MHz.
    //
    // 16 MHz = 16,000,000 clock cycles per second.
    //
    // Therefore:
    //
    // Number of clock cycles for 1 second
    // = 16,000,000
    //
    // SysTick counts DOWN from the LOAD value to 0.
    //
    // Therefore:
    //
    // LOAD = 16,000,000 - 1
    //      = 15,999,999


    SysTick->LOAD = 16000000 - 1;


    // =========================================================
    // 4. CLEAR CURRENT SYSTICK VALUE
    // =========================================================
    //
    // VAL contains the current value of the SysTick counter.
    //
    // Setting VAL to 0 clears the current counter value.
    //
    // The timer will then start using the LOAD value.

    SysTick->VAL = 0;


    // =========================================================
    // 5. ENABLE AND CONFIGURE SYSTICK
    // =========================================================
    //
    // CTRL is the SysTick control register.
    //
    // We set CTRL = 7.
    //
    // 7 in binary is:
    //
    // 111
    //
    // The important bits are:
    //
    // Bit 0 = ENABLE
    // Bit 1 = TICKINT
    // Bit 2 = CLKSOURCE
    //
    // Therefore:
    //
    // Bit 0 = 1 → Enable SysTick timer
    // Bit 1 = 1 → Enable SysTick interrupt
    // Bit 2 = 1 → Use processor clock
    //
    // So CTRL = 7 enables all three.


    SysTick->CTRL = 7;


    // =========================================================
    // 6. MAIN LOOP
    // =========================================================
    //
    // We don't need to manually create a delay here.
    //
    // SysTick is running automatically.
    //
    // After every 1 second:
    //
    // SysTick reaches 0
    //        ↓
    // Interrupt occurs
    //        ↓
    // SysTick_Handler() executes
    //        ↓
    // PC14 is toggled
    //
    // Then the processor returns to this loop.

    while (1)
    {
        // Nothing is required here.
        // SysTick handles the timing and LED toggling.
    }
}


// =============================================================
// SYSTICK INTERRUPT HANDLER
// =============================================================
//
// This function is automatically called whenever
// the SysTick timer reaches 0.
//
// Since LOAD was set for 1 second:
//
// SysTick_Handler() runs every 1 second.
// =============================================================

void SysTick_Handler(void)
{
    // =========================================================
    // TOGGLE PC14
    // =========================================================
    //
    // ODR = Output Data Register.
    //
    // Bit 14 of ODR controls PC14.
    //
    // XOR (^) with 1 toggles a bit:
    //
    // 0 XOR 1 = 1
    // 1 XOR 1 = 0
    //
    // Therefore:
    //
    // If PC14 is LOW  → it becomes HIGH
    // If PC14 is HIGH → it becomes LOW
    //
    // This makes the LED alternate between ON and OFF.

    GPIOC->ODR ^= (1U << 14);
}