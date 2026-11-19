# Hardware Architecture Audit & Conflict Analysis

## 1. Overview
This audit examines the hardware architecture of ForgeSense and individual peripheral experiments based on:
1. The physical connection diagram (`historical-source/attachments/ForgeSense_Architecture_Diagram.jpg`).
2. The case study report (`10_Projects/ForgeSense/documentation/ForgeSense_Final_Case_Study_Report.pdf`).
3. Historical firmware implementations.

---

## 2. Identified Hardware Discrepancies and Conflicts

### Issue 1: L298N Dual Pin Duplication & UART PA9 Conflict (Critical)
* **Diagram Assignment**: 
  - `IN1 -> PA8`, `IN2 -> PA9`
  - `IN3 -> PA8`, `IN4 -> PA9`
  - `ESP8266 TX -> PA9` (USART1_TX)
* **Root Cause**:
  1. Tying Channel A (IN1, IN2) and Channel B (IN3, IN4) directly together causes redundant switching and prevents independent motor control.
  2. Pin `PA9` cannot simultaneously serve as a push-pull GPIO direction output for the L298N and as `USART1_TX` (AF7) without electrical bus contention and serial corruption.
* **Resolution**:
  - Dedicate Channel B to the DC motor: `PA6` (TIM3_CH1 PWM to ENB), `PA8` (IN3), and `PA9` (IN4).
  - Migrate serial communications from USART1 to **USART2**:
    - `PA2` -> `USART2_TX` (AF7) connected to ESP8266 RX.
    - `PA3` -> `USART2_RX` (AF7).
  - Move Current Sensor from `PA2` to `PA5` (`ADC1_IN5`) to avoid conflict with USART2.

### Issue 2: ESP8266 Serial TX/RX Direction
* **Historical Conflict**: Earlier descriptions inverted ESP8266 TX/RX assignments.
* **Resolution**:
  - Microcontroller TX must connect to Peripheral RX:
    - `STM32 PA2 (USART2_TX)` -> `ESP8266 RX0 (GPIO3)`
    - `STM32 PA3 (USART2_RX)` -> `ESP8266 TX0 (GPIO1)`
  - Logic levels: STM32 3.3V I/O is directly compatible with ESP8266 3.3V logic. Common ground is mandatory.

### Issue 3: Emergency Stop Input vs. Status Indication
* **Historical Conflict**: Diagram specifies `PB12` as Emergency Stop Button, whereas early reports mentioned `PB5`. Additionally, `PC13` was referenced as both an emergency input and an indicator.
* **Resolution**:
  - `PB12`: Dedicated hardware Emergency Stop Button (Active-LOW with internal pull-up).
  - `PB5`: Secondary / Normal operator button.
  - `PC13`: Dedicated active-LOW onboard LED used for rapid blinking (5 Hz) in safety lockout mode.

### Issue 4: Buzzer Polarity
* **Specification**: Active-LOW piezo buzzer on `PB9`.
* **Verification**:
  - High level (3.3V via `GPIOB->BSRR = (1U << 9)`) = Deactivated (Silent).
  - Low level (0V via `GPIOB->BSRR = (1U << (9 + 16))`) = Activated (Sounding).
