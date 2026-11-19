# Common Embedded C & STM32 Bare-Metal Errors

## 1. Top 5 Traps in STM32F4 Register Programming

### 1. EXTI Pending Register Read-Modify-Write
* **Bug**: `EXTI->PR |= (1 << line);`
* **Why it fails**: `EXTI->PR` is "rc_w1" (Read/Clear by writing '1'). Writing a `1` clears the pending flag; writing a `0` leaves it unaffected. If another interrupt line is pending when you read `EXTI->PR`, its bit will be `1`. The OR operation preserves that `1` in the write-back, which accidentally clears that unrelated interrupt before its ISR can run.
* **Fix**: Use direct assignment: `EXTI->PR = (1U << line);`.

### 2. ADC EOC Polling on Control Register
* **Bug**: `while (!(ADC1->CR2 & (1 << 1)));`
* **Why it fails**: Bit 1 of `CR2` is `CONT` (continuous mode enable), which is static configuration. The End-of-Conversion flag is bit 1 of `ADC1->SR` (`ADC_SR_EOC`).
* **Fix**: `while ((ADC1->SR & ADC_SR_EOC) == 0U);`.

### 3. Active-LOW Output Logic Inversion
* **Bug**: Driving a GPIO HIGH to sound an active-LOW buzzer.
* **Why it fails**: Active-LOW devices conduct when the output is driven to 0V (ground sink). Driving the pin HIGH deactivates the transistor or piezo buzzer.
* **Fix**:
  - `GPIOB->BSRR = (1U << (pin + 16U));` (LOW = Active / Sounding).
  - `GPIOB->BSRR = (1U << pin);` (HIGH = Inactive / Silent).

### 4. Missing Timer Preload or Update Generation
* **Bug**: Changing PWM duty cycle in `CCR1` without generating an update event or enabling preload (`ARPE`/`OC1PE`).
* **Why it fails**: Glitches can occur mid-period if compare registers update asynchronously.
* **Fix**: Always set `OC1PE` in `TIMx->CCMR1` and `ARPE` in `TIMx->CR1`.

### 5. Open-Drain I2C Bus Hangs
* **Bug**: I2C master waits indefinitely in `while (I2C1->SR2 & I2C_SR2_BUSY);`.
* **Why it fails**: Missing external pull-up resistors or an unacknowledged slave can latch the SDA line LOW.
* **Fix**: Implement timeouts on every status flag check and perform bus clear sequences if stuck.
