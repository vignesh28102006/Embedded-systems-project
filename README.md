# Embedded Systems STM32: Coursework Analysis, Code Audit & Firmware Architecture

[![Target: STM32F401CCU6](https://img.shields.io/badge/MCU-STM32F401CCU6-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f401cc.html)
[![Core: ARM Cortex-M4](https://img.shields.io/badge/Core-ARM%20Cortex--M4%20with%20FPU-orange.svg)](https://developer.arm.com/Processors/Cortex-M4)
[![Language: Embedded C](https://img.shields.io/badge/Language-Embedded%20C%20(Bare--metal)-green.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

A rigorous, professional, bare-metal Embedded C repository documenting, auditing, correcting, and validating the complete Embedded Systems coursework, laboratory practicals, and the flagship **ForgeSense** industrial machine monitoring project built on the **STM32F401CCU6** ("Black Pill") development platform.

---

## 1. Primary Objectives & Methodology
* **Bare-Metal Register Programming**: Direct peripheral manipulation via CMSIS device headers (`stm32f4xx.h`) without relying on abstraction layers (no HAL/LL libraries).
* **Historical Code Preservation**: Original source files preserved in `historical-source/original/` and audited without destructive overwrites.
* **Rigorous Defect Audit**: Technical root-cause identification across EXTI pending bit handling, ADC polling traps, pin allocation conflicts, and timer PWM calculations.
* **Reconciled Systems Engineering**: Elimination of hardware resource contentions (e.g. USART1 vs L298N pin collisions) and delivery of verified reference firmware.

---

## 2. Hardware Architecture & Development Tools
- **Microcontroller**: STM32F401CCU6 (ARM Cortex-M4 @ 16 MHz HSI / up to 84 MHz PLL)
- **Development Board**: WeAct Black Pill
- **Toolchains**: Keil MDK / µVision, ARM GNU Toolchain (`arm-none-eabi-gcc`), OpenOCD / ST-Link V2
- **Key Hardware Modules**:
  - L298N Dual H-Bridge Motor Driver with DC Geared Motor
  - SSD1306 0.96" I2C OLED Display (`0x3C`)
  - PCF8574 16x2 Character LCD Backpack (`0x4E`)
  - MPU6050 6-DOF I2C Accelerometer / Gyroscope (`0x68`)
  - ESP8266 ESP-01 Serial Wi-Fi Transceiver (9600 Baud Telemetry)
  - LM35 Precision Centigrade Temperature Sensor ($10\text{ mV/}^\circ\text{C}$)
  - ACS712-05A Hall-Effect Linear Current Sensor
  - Resistor Voltage Divider for Battery / Supply Monitoring
  - Active-LOW Piezo Buzzer & 5x External Status LEDs

---

## 3. Repository Directory Structure

```text
Embedded-Systems-STM32/
├── README.md                           # Master Project Documentation & Index
├── LICENSE                             # MIT Open Source License
├── CONTRIBUTING.md                      # Code Style & Contribution Guidelines
├── CHANGELOG.md                        # Version & Defect Correction History
│
├── 00_Introduction/                    # MCU Overview & Toolchain Setup
│   └── STM32F401CCU6_Introduction.md
│
├── 01_ARM_Cortex_M4/                   # Cortex-M4 Architecture & Core Theory
│   ├── Architecture.md                 # Harvard Bus, Registers, Dual Stack, NVIC
│   └── 3b. ARM architecture_part2.pptx  # Preserved Lecture Material
│
├── 02_GPIO/                            # General Purpose I/O Theory & Practicals
├── 03_ADC/                             # Analog-to-Digital Converter Engineering
├── 04_Communication/                   # USART & I2C Master Implementations
├── 05_Interrupts/                      # EXTI External Lines & NVIC Configuration
├── 06_SysTick_Timer/                   # Core SysTick Delay & Interval Generation
├── 07_PWM/                             # TIM3 PWM Generation & H-Bridge Control
├── 08_Display_Interfacing/             # I2C OLED (SSD1306) & LCD (PCF8574)
├── 09_Sensor_Interfacing/              # LM35, ACS712, Voltage Divider, MPU6050
│
├── 10_Projects/                        # Coursework Applications
│   ├── ForgeSense/                     # Flagship Machine Condition Monitor
│   │   ├── README.md
│   │   ├── hardware/                   # Circuit & Pin Interconnect Specs
│   │   ├── firmware/
│   │   │   ├── original/               # Historical motor control implementation
│   │   │   ├── modified/               # Version with MPU6050 vibration logic
│   │   │   ├── corrected/              # Audited motor control program
│   │   │   └── final/                  # Final production integrated firmware
│   │   └── documentation/              # Original Case Study Report PDF
│   ├── Railway_Crossing/               # Curriculum Placeholder
│   ├── Smart_Water_Level/              # Curriculum Placeholder
│   └── Smart_Security_Light/           # Curriculum Placeholder
│
├── 11_Lab_Exercises/                   # Complete Practical Laboratory Catalog
│   └── README.md
│
├── 12_Debugging/                       # Embedded C Traps & Register Debugging
│   └── Common_Errors.md
│
├── 13_Reference_Tables/                # Hardware Pin & Register Mapping
│   ├── GPIO_Pin_Mapping.md
│   └── Register_Bit_Positions.md
│
├── 14_Circuit_Diagrams/                # Schematic Diagrams & Photos
│   └── ForgeSense/
│       └── ForgeSense_Architecture_Diagram.jpg
│
├── 15_Datasheets/                      # Official ST Reference Manuals
│   └── STM32F401/
│       └── RM0368_STM32F401_Reference_Manual.pdf
│
├── 16_Audit_Reports/                   # Technical Defect & Validation Reports
│   ├── Code_Audit.md                   # Source-level bug breakdown
│   ├── Hardware_Audit.md               # Hardware pin contention analysis
│   ├── Integration_Audit.md            # Module dependency & safety interlock
│   └── Validation_Report.md            # Verification status matrix
│
└── historical-source/                  # Strict Code Preservation Archive
    ├── original/                       # Untouched historical C source files
    ├── modified/                       # Intermediate student iterations
    └── attachments/                    # Raw incoming media & PDFs
```

---

## 4. Primary Project: ForgeSense Architecture

ForgeSense acquires multi-channel sensor parameters and controls machine actuation with an autonomous high-priority safety interlock:

![ForgeSense Architecture Diagram](14_Circuit_Diagrams/ForgeSense/ForgeSense_Architecture_Diagram.jpg)

### Key Hardware Reconciliations:
1. **USART2 Migration**: Migrated Wi-Fi telemetry from `PA9`/`PA10` (USART1) to `PA2`/`PA3` (USART2) to resolve a direct physical pin collision with the L298N motor direction input on `PA9`.
2. **Current Sensor Mapping**: Moved ACS712 analog input to `PA5` (`ADC1_IN5`), freeing `PA2` for `USART2_TX`.
3. **Emergency Stop Interlock**: Dedicated active-LOW switch on `PB12` triggering immediate PWM zeroing, motor de-energization, buzzer silencing, display zeroing, and a latching 5 Hz lockout blink on `PC13` and `PB0`.

---

## 5. Technical Validation Summary

| Component | Static Code Review | Register Mapping Review | Toolchain Build | Hardware Field Test |
|:---|:---:|:---:|:---:|:---:|
| **ForgeSense Final Integrated Firmware** | **Passed** | **Passed** | *Available via Keil / arm-gcc* | *UNVERIFIED (Requires physical breadboard)* |
| **Potentiometer Motor Control** | **Passed** | **Passed** | *Verified (Historical Flash Log)* | *Verified (Historical Flash Log)* |
| **I2C1 LCD (PCF8574) & ADC (`bh12c.c`)** | **Passed** | **Passed** | *Passed* | *UNVERIFIED* |
| **Dual EXTI Interrupts (`GPIO_Interupts.c`)**| **Passed** | **Passed** | *Passed* | *UNVERIFIED* |
| **SysTick Busy-Wait (`syswt.c`)** | **Passed** | **Passed** | *Passed* | *UNVERIFIED* |

*Note*: As per safety guidelines, all newly written firmware is distinctly categorized from untouched historical code.
