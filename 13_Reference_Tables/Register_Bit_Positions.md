# Register Bit Positions & Bitwise Operations Reference

## 1. Bitwise Manipulation Principles

In STM32 bare-metal development:
- **Pin Mode Calculation**: Each GPIO pin has 2 bits in `MODER`.
  $$\text{Bit Position} = \text{pin} \times 2$$
  - Clear mask: `~(3U << (pin * 2))`
  - Set mode: `(mode << (pin * 2))`
- **Alternate Function Calculation**: Each pin has 4 bits in `AFR[0]` (pins 0–7) or `AFR[1]` (pins 8–15).
  $$\text{AFR Index} = \text{pin} / 8, \quad \text{Bit Position} = (\text{pin} \pmod 8) \times 4$$
  - Clear mask: `~(0xFU << ((pin % 8) * 4))`
  - Set AF: `(af_num << ((pin % 8) * 4))`
- **ADC Sampling Time**: Each channel has 3 bits in `SMPR2` (channels 0–9) or `SMPR1` (channels 10–18).
  $$\text{Bit Position} = (\text{channel} \pmod{10}) \times 3$$
  - Sample time 84 cycles: `(4U << (channel * 3))`

---

## 2. Baud Rate Calculation for USART

The baud rate register (`BRR`) encodes the clock division factor:
$$\text{USARTDIV} = \frac{f_{PCLK}}{16 \times \text{Baud Rate}} \quad (\text{when OVER8 = 0})$$

For USART2 connected to APB1 ($f_{PCLK1} = 16\text{ MHz}$) at 9600 baud:
$$\text{USARTDIV} = \frac{16,000,000}{16 \times 9600} = \frac{1,000,000}{9600} \approx 104.16667$$

* **Mantissa**: $\lfloor 104.16667 \rfloor = 104 = 0\text{x}68$
* **Fraction**: $\text{round}(0.16667 \times 16) = \text{round}(2.6667) = 3 = 0\text{x}3$
* **BRR Value**:
  $$\text{BRR} = (104 \ll 4) | 3 = 0\text{x}683 = 1667\text{ decimal}$$

*Actual Baud Rate Achieved*:
$$\text{Actual Baud} = \frac{16,000,000}{16 \times 104.1875} \approx 9598.08\text{ baud} \quad (\text{Error: } -0.02\%)$$

---

## 3. SysTick Delay Calculation

Core clock $f_{HCLK} = 16\text{ MHz}$.
SysTick reload value for a 1 millisecond interval ($1\text{ kHz}$ count rate):
$$\text{LOAD} = (16,000,000 \times 0.001) - 1 = 16,000 - 1 = 15,999$$

For a 1 second delay interval:
$$\text{LOAD} = (16,000,000 \times 1.0) - 1 = 15,999,999$$

SysTick control register (`CTRL`):
- Bit 0 (`ENABLE`): 1 = Counter enabled
- Bit 1 (`TICKINT`): 1 = Exception enabled, 0 = Polling
- Bit 2 (`CLKSOURCE`): 1 = Processor clock ($f_{HCLK}$), 0 = External reference clock
- Bit 16 (`COUNTFLAG`): 1 = Counter has counted down to 0 since last read
