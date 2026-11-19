# ForgeSense Hardware Architecture & Pin Interconnect Specification

## 1. System Overview
ForgeSense is an intelligent machine condition monitoring and motor drive supervisory platform developed on the STM32F401CCU6 (ARM Cortex-M4 @ 16 MHz HSI).

![ForgeSense Architecture Diagram](../../14_Circuit_Diagrams/ForgeSense/ForgeSense_Architecture_Diagram.jpg)

---

## 2. Definitive Hardware Pin Mapping

| Peripheral / Sensor | Signal | STM32 Pin | Function / AF | Electrical Specs / Signal Range | Logic / Action |
|:---|:---|:---:|:---|:---|:---|
| **Speed Potentiometer** | Wiper | `PA0` | `ADC1_IN0` (Analog) | 0.0 V – 3.3 V | Speed setpoint (0–100% duty) |
| **LM35 Temperature** | Analog Vout | `PA1` | `ADC1_IN1` (Analog) | $10\text{ mV/}^\circ\text{C}$ ($250\text{ mV} = 25^\circ\text{C}$) | Ambient/motor temperature monitor |
| **Battery Divider** | Scaled Vout | `PA4` | `ADC1_IN4` (Analog) | 1:2 Divider ($6.6\text{ V max}$ scaled to $3.3\text{ V}$) | Supply voltage monitor |
| **ACS712 Current Sensor**| Vout | `PA5` | `ADC1_IN5` (Analog) | $185\text{ mV/A}$ ($V_{CC}/2$ zero offset) | Motor current monitoring |
| **L298N PWM Enable** | ENA / ENB | `PA6` | `TIM3_CH1` (AF2) | 3.3 V Logic, 1 kHz PWM | Speed control (0–999 count) |
| **L298N Direction 1** | IN3 | `PA8` | GPIO Output | 3.3 V Push-pull | Motor Direction A |
| **L298N Direction 2** | IN4 | `PA9` | GPIO Output | 3.3 V Push-pull | Motor Direction B |
| **ESP8266 Wi-Fi TX** | RX0 (ESP) | `PA2` | `USART2_TX` (AF7) | 3.3 V Asynchronous Serial | 9600 baud, 8-N-1 CSV telemetry |
| **ESP8266 Wi-Fi RX** | TX0 (ESP) | `PA3` | `USART2_RX` (AF7) | 3.3 V Asynchronous Serial | Command reception |
| **OLED Display SCL** | SCL | `PB6` | `I2C1_SCL` (AF4) | Open-drain with external 4.7 kΩ pull-up | 100 kHz Standard Mode I2C |
| **OLED Display SDA** | SDA | `PB7` | `I2C1_SDA` (AF4) | Open-drain with external 4.7 kΩ pull-up | Shared with MPU6050 (`0x68`) |
| **Active-LOW Buzzer** | Control | `PB9` | GPIO Output | 3.3 V Push-pull | **0 = ON (Sound), 1 = OFF (Silent)** |
| **Status LED Green** | Anode | `PB0` | GPIO Output | 3.3 V with 220 Ω series resistor | Normal running state |
| **Status LED Yellow** | Anode | `PB1` | GPIO Output | 3.3 V with 220 Ω series resistor | Warning threshold reached |
| **Status LED Red** | Anode | `PB2` | GPIO Output | 3.3 V with 220 Ω series resistor | Fault / Lockout alert |
| **Normal Pushbutton** | Switch | `PB5` (or `PA12`) | GPIO Input | Pulled UP internally (active-LOW) | Mode toggle / Operator input |
| **Emergency Stop Button**| E-Stop N.O. | `PB12` | GPIO Input | Pulled UP internally (active-LOW) | **High-priority Safety Interlock** |
| **Onboard Debug LED** | LED | `PC13` | GPIO Output | Active-LOW (driven by Black Pill) | Rapid 5 Hz emergency blink |

---

## 3. Power Supply Distribution & Electrical Isolation
* **Logic Supply (3.3V)**: Sourced from the onboard AMS1117-3.3 regulator powered via USB-C or external 5V rail.
* **Motor Supply (9V–12V)**: Fed directly to the L298N power terminal block. The L298N 5V onboard regulator jumper can remain fitted only if logic input is within limits.
* **Common Grounding**: STM32 GND, L298N GND, ESP8266 GND, and battery negative MUST be bonded to a single ground plane.
