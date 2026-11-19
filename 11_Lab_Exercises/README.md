# Laboratory Exercises & Coursework Practical Experiments

This section documents all recovered hands-on laboratory exercises and mid-semester practical examinations conducted on the STM32F401 Black Pill platform.

---

## Index of Recovered Practical Experiments

| Exercise ID | Experiment Name | Primary Peripheral | Source File in `historical-source/original/` | Objectives & Summary |
|:---:|:---|:---:|:---|:---|
| **LAB-01** | Basic GPIO Output & LED Blinking | GPIO | `blink.c`, `pc13.c`, `pc14.c` | Basic AHB1 bus clock enabling, MODER configuration, and atomic pin toggling. |
| **LAB-02** | Pushbutton Input Polling & Debouncing | GPIO | `switch.c` | Active-LOW button reading, PUPDR configuration, and software delay debouncing. |
| **LAB-03** | Dual EXTI External Interrupts | EXTI / NVIC | `GPIO_Interupts.c`, `GPIO.c` | SYSCFG EXTI mapping on PA6 and PB10, falling-edge trigger, NVIC vector enabling. |
| **LAB-04** | SysTick 1 Hz Interval Generation | SysTick | `syswt.c`, `q1.c`, `Systick_without_interupt.txt` | Core timer reload calculation, busy-wait polling COUNTFLAG, and 1 Hz PC14 toggling. |
| **LAB-05** | Single-Channel ADC Ingestion | ADC1 | `ADC.c` | PA0 analog mode, software triggering with SWSTART, continuous conversion polling. |
| **LAB-06** | I2C1 16x2 LCD Character Display | I2C1 | `i2c.c`, `sji2c.c`, `Potentio_LCD_ADC.txt` | Standard mode 100 kHz I2C master driver for PCF8574 I2C backpack. |
| **LAB-07** | Integrated Multi-Peripheral System | I2C / ADC / SysTick / EXTI | `bh12c.c` | Full coursework integration: I2C LCD + PA0 ADC + SysTick blink + PB2 EXTI mode toggle. |
| **EXAM-01**| Mid-Semester Exam Practical 1 | ADC / GPIO | `Q1MS.c` | Multi-threshold ADC window comparator (0–1023, 1024–2047, 2048–3071) with PC13 rate blink. |
| **EXAM-02**| Mid-Semester Exam Practical 2 | ADC / EXTI / Buzzer | `Q2MS.c`, `q1mspyq.c` | Pushbutton interlocks, ADC threshold comparison (<800 vs >800), and PB9 buzzer alarm. |

---

## Detailed Write-Up: Integrated Lab 07 (`bh12c.c`)

### 1. Aim
Integrate an analog potentiometer input, an external interrupt toggle button, an I2C character LCD, and a SysTick background timer into a cohesive embedded monitoring system without RTOS.

### 2. Hardware Connections
- **Potentiometer**: Connected to `PA0` (`ADC1_IN0`).
- **Pushbutton**: Connected to `PB2` with internal pull-up (`EXTI2`).
- **I2C LCD**: `PB6` (SCL) and `PB7` (SDA) wired to PCF8574 adapter (address `0x4E`).
- **Status LED**: Connected to `PC14`.

### 3. Key Design Verification
- `EXTI2_IRQHandler` correctly clears its pending bit with write-one-to-clear: `EXTI->PR = (1U << 2);`.
- ADC End-of-Conversion is serviced in `ADC_IRQHandler` via interrupt vector `ADC_IRQn`.
- SysTick generates a 1 ms tick for LCD timing and 1 Hz LED blink pacing.
