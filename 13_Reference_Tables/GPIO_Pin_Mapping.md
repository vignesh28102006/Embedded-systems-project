# STM32F401CCU6 Peripheral Reference Tables

## 1. GPIO Pin Interconnect Master Table

| Port & Pin | Mode | Alternate Function | Peripheral Mapping | ForgeSense Connection | Notes |
|:---:|:---:|:---:|:---:|:---:|:---|
| `PA0` | Analog (11b) | — | `ADC1_IN0` | Potentiometer Speed Setpoint | Linear 0 to 3.3V analog input |
| `PA1` | Analog (11b) | — | `ADC1_IN1` | LM35 Temperature Sensor | 10 mV/°C analog output |
| `PA2` | AF (10b) | `AF7` | `USART2_TX` | ESP8266 RX0 (Telemetry) | Reconciled from PA9 to resolve conflict |
| `PA3` | AF (10b) | `AF7` | `USART2_RX` | ESP8266 TX0 (Receive) | 3.3V serial logic |
| `PA4` | Analog (11b) | — | `ADC1_IN4` | Battery Voltage Divider | 1:2 resistor divider (6.6V max) |
| `PA5` | Analog (11b) | — | `ADC1_IN5` | ACS712 Current Sensor | Analog current output |
| `PA6` | AF (10b) | `AF2` | `TIM3_CH1` | L298N ENA/ENB (Motor PWM) | 1 kHz frequency, 0–100% duty |
| `PA8` | Output (01b)| — | GPIO Push-Pull | L298N Direction IN3 | Forward/Reverse motor logic |
| `PA9` | Output (01b)| — | GPIO Push-Pull | L298N Direction IN4 | Forward/Reverse motor logic |
| `PA12`| Input (00b) | — | GPIO Pull-Up | Secondary Operator Button | Active-LOW input |
| `PB0` | Output (01b)| — | GPIO Push-Pull | Red Status LED | 220 Ω series resistor |
| `PB1` | Output (01b)| — | GPIO Push-Pull | Green Status LED | 220 Ω series resistor |
| `PB2` | Output (01b)| — | GPIO Push-Pull | Yellow Status LED | 220 Ω series resistor |
| `PB5` | Input (00b) | — | GPIO Pull-Up | Normal Operator Button | Active-LOW input |
| `PB6` | AF (10b) | `AF4` | `I2C1_SCL` | SSD1306 OLED & MPU6050 SCL | Open-drain with external 4.7 kΩ pull-up |
| `PB7` | AF (10b) | `AF4` | `I2C1_SDA` | SSD1306 OLED & MPU6050 SDA | Open-drain with external 4.7 kΩ pull-up |
| `PB9` | Output (01b)| — | GPIO Push-Pull | Active-LOW Piezo Buzzer | 1 = Silent, 0 = Active |
| `PB10`| Output (01b)| — | GPIO Push-Pull | Blue Status LED | 220 Ω series resistor |
| `PB11`| Output (01b)| — | GPIO Push-Pull | Orange Status LED | 220 Ω series resistor |
| `PB12`| Input (00b) | — | GPIO Pull-Up | **Emergency Stop Button** | **High-priority Safety Interlock** |
| `PC13`| Output (01b)| — | GPIO Open-Drain | Onboard User LED | Active-LOW, rapid emergency blink |

---

## 2. ADC1 Regular Sequence Channel Map

| Channel | Pin | Sampling Time Bits (`SMPR2` / `SMPR1`) | Typical Clocks | Function in Firmware |
|:---:|:---:|:---:|:---:|:---|
| `IN0` | `PA0` | `SMPR2[2:0] = 100b` | 84 cycles | Speed potentiometer setpoint |
| `IN1` | `PA1` | `SMPR2[5:3] = 100b` | 84 cycles | LM35 motor/ambient temp |
| `IN4` | `PA4` | `SMPR2[14:12] = 100b` | 84 cycles | Battery voltage divider |
| `IN5` | `PA5` | `SMPR2[17:15] = 100b` | 84 cycles | ACS712 current sensor |

*Note*: ADC Clock Prescaler is set to `DIV2` (`ADC->CCR &= ~(3U << 16)`), resulting in an 8 MHz ADC clock at 16 MHz $PCLK_2$, safely within the 36 MHz datasheet maximum.
