# Changelog & Technical Audit History

All notable technical fixes, hardware reconciliations, and architectural changes in this repository are documented below.

## [1.0.0] - 2026-11-19

### Discovered & Preserved
- Preserved all 17 original coursework C programs from `Documents/sem5/ES` into `historical-source/original/`.
- Preserved `potentiometer_dc_motor_control.c` in `historical-source/original/` and `10_Projects/ForgeSense/firmware/original/`.
- Preserved intermediate ForgeSense firmware iterations (`ForgeSense_STM32_UART_v1.c` and `ForgeSense_STM32_MPU6050_v2.c`) in `historical-source/modified/`.
- Preserved official Case Study Report PDF and hardware architecture diagram in `historical-source/attachments/`.

### Audited & Corrected
- **Hardware Conflict**: Reconciled L298N direction pin `PA9` collision with USART1 by migrating wireless telemetry to `USART2` (`PA2` TX, `PA3` RX).
- **ADC Collision**: Shifted ACS712 current measurement to `PA5` (`ADC1_IN5`), freeing `PA2` for serial transmission.
- **EXTI Clearing**: Fixed destructive read-modify-write on `EXTI->PR` across `GPIO.c` and `GPIO_Interupts.c`, replacing `EXTI->PR |= mask` with write-one-to-clear `EXTI->PR = mask`.
- **ADC EOC Polling**: Fixed invalid polling on control register `ADC1->CR2` across exam programs (`Q1MS.c`, `q1mspyq.c`), updating check to `ADC1->SR & ADC_SR_EOC`.
- **Motor Failsafe**: Corrected `potentiometer_dc_motor_control.c` to shut down motor PWM and de-energize direction outputs upon conversion timeout.
- **Buzzer Polarity**: Verified and validated active-LOW buzzer control on `PB9`.
