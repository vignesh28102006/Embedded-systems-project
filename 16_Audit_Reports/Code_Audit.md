# Code Audit & Firmware Defect Analysis

## 1. Scope of Audit
This audit examines all recoverable historical C files across `historical-source/original/`, identifying syntax errors, register misconfigurations, race conditions, and architectural bugs.

---

## 2. Detailed Findings by Module

### 2.1 EXTI & NVIC Configuration (`GPIO.c`, `GPIO_Interupts.c`)
* **Defect 1: Destructive Read-Modify-Write on Pending Register**
  - *Location*: `GPIO_Interupts.c:58`, `GPIO.c:46` (`EXTI->PR |= (1 << 6);`)
  - *Root Cause*: `EXTI->PR` is "rc_w1" (Read/Clear by writing '1'). A read-modify-write operation (`|=`) reads existing '1's for other pending interrupts and writes them back, accidentally clearing unrelated pending flags before their ISRs execute.
  - *Correction*: Direct write: `EXTI->PR = (1U << 6);`.
* **Defect 2: Syntax and Symbol Typos**
  - *Location*: `GPIO.c:33-34`, `42`
  - *Root Cause*: Written as `NVIC_EnableIQR(EXTI9_5_IQRn)` and `NVIC_EnavleIQR(EXTI15_10_IQRn)` instead of `NVIC_EnableIRQ` and `IRQn`. Function declared as `EXTI9_5_IQRHandler` instead of `EXTI9_5_IRQHandler`.
  - *Correction*: Fix identifier spellings.
* **Defect 3: Register Overwrite in FTSR**
  - *Location*: `GPIO.c:30-31` (`EXTI->FTSR = (1<<6); EXTI->FTSR = (1<<10);`)
  - *Root Cause*: Direct assignment (`=`) instead of bitwise OR (`|=`) causes line 6 trigger configuration to be overwritten by line 10.
  - *Correction*: Use `EXTI->FTSR |= (1U << 6) | (1U << 10);`.

### 2.2 ADC Conversion & Polling (`Q1MS.c`, `q1mspyq.c`, `Q2MS.c`, `ADC.c`)
* **Defect 1: Polling Control Register Instead of Status Register**
  - *Location*: `Q1MS.c:29`, `q1mspyq.c:43` (`while(!(ADC1->CR2 & (1U<<1))){}`)
  - *Root Cause*: `CR2` bit 1 is `CONT` (continuous conversion mode bit), not `EOC`. Polling this register never detects conversion completion.
  - *Correction*: Poll `ADC1->SR & ADC_SR_EOC` (bit 1 of `ADC_SR`).
* **Defect 2: Assignment Operator in While Loop**
  - *Location*: `Q1MS.c:23`, `q1mspyq.c:37` (`while((ADC1->CR2 = 1U) == 0){}`)
  - *Root Cause*: Assignment `=` used instead of bitwise test `&`. This repeatedly resets `ADC1->CR2` to `1`.
  - *Correction*: Test bit using `while((ADC1->CR2 & ADC_CR2_ADON) == 0U){}`.
* **Defect 3: Invalid Register Access**
  - *Location*: `Q1MS.c:15-16` (`ADC1->CCR1 = 0; ADC1->CCR2 = 0;`)
  - *Root Cause*: `CCR1` and `CCR2` do not exist in the STM32F4 ADC peripheral (`CR1` and `CR2` are the control registers; `ADC->CCR` is the common control register).
  - *Correction*: Reference `ADC1->CR1 = 0U; ADC1->CR2 = 0U;` and `ADC->CCR = 0U;`.

### 2.3 Potentiometer Motor Control (`potentiometer_dc_motor_control.c`)
* **Defect 1: Timeout Fallback Returns Stale Data**
  - *Location*: `Read_Potentiometer()`
  - *Root Cause*: If conversion times out, the function returns the last valid `potValue`. If the sensor disconnects or ADC hangs, the motor continues spinning at its last commanded duty cycle.
  - *Correction*: Return a distinct error code or 0, and immediately assert a safe motor stop condition.
* **Defect 2: Non-Zero Starting Duty and Boundary Behavior**
  - *Behavior*: Threshold stops motor when `adcValue >= 2500`. Below 2500, motor runs forward. When `adcValue == 0`, duty is 0%, but direction pins remain energized.
  - *Correction*: Explicitly de-energize direction pins when commanded PWM is zero.

### 2.4 Header Typo (`blink.c`, `pc14.c`, `pb9.c`)
* **Defect**: `#include "stm32fxx.h"` missing the numeral `4`.
* **Correction**: `#include "stm32f4xx.h"`.
