# STM32F401CCU6 & Black Pill Architecture Guide

## 1. Device Specifications (STM32F401CCU6)
- **Core**: ARM 32-bit Cortex-M4 with FPU @ up to 84 MHz maximum frequency.
- **Flash Memory**: 256 KB.
- **SRAM**: 64 KB.
- **Supply Voltage**: 1.7 V to 3.6 V.
- **Package**: UFQFPN48 (48 pins).
- **ADC**: 1x 12-bit, 2.4 MSPS ADC with 16 channels. Max permitted ADC clock: 36 MHz.
- **Timers**: Up to 11 timers (TIM1 advanced 16-bit, TIM2/TIM5 32-bit general purpose, TIM3/TIM4 16-bit general purpose, SysTick 24-bit downcounter).
- **Communication Interfaces**:
  - Up to 3x I2C interfaces (SMBus/PMBus).
  - Up to 3x USARTs (USART1 up to 10.5 Mbit/s, USART2/6 up to 5.25 Mbit/s).
  - Up to 4x SPI interfaces.
  - USB 2.0 full-speed device/host/OTG controller.

---

## 2. WeAct Black Pill Board Layout
- **Crystal Oscillators**:
  - Main High-Speed External (HSE): 25.000 MHz.
  - Low-Speed External (LSE): 32.768 kHz RTC crystal.
- **User Peripherals**:
  - User Key (Button): `PA0` (Active-HIGH, pulled down externally).
  - User LED: `PC13` (Active-LOW, sink configuration).
  - Reset (NRST) Button.
- **Power Configuration**:
  - 3.3V LDO regulator on board (input 5V from USB-C or VIN).
  - Header pins 3.3V, 5V, and GND.
