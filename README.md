# Environmental Monitoring System — STM32 Nucleo-F446RE

Bare-metal HAL environmental monitor on the Nucleo-F446RE. Reads a BMP280 (temp/pressure) over I2C, uses a potentiometer as a live alert threshold, runs a 3-state FSM with hysteresis, mirrors status to a 16x2 LCD, and exposes a UART shell. Built with Makefile + arm-none-eabi-gcc + OpenOCD (macOS, no CubeIDE).

## Features

- UART shell, interrupt-driven RX, 115200 8N1
- ADC potentiometer (PA0) as a live alert threshold — no reflash needed
- BMP280 over I2C1: calibration readout, datasheet compensation, write-verified register writes on init
- TIM2 1 Hz sampling via ISR flag, consumed in the main loop
- 16x2 HD44780 LCD, bit-banged 4-bit parallel, mirrors UART output
- 3-state FSM (`OK` / `ALERT` / `SENSOR_ERROR`) with hysteresis and edge-triggered LED
- All routed through one shared `Sensors_Sample_And_Report()`

## Hardware

- STM32 Nucleo-F446RE
- BMP280 breakout (I2C)
- 16x2 HD44780 LCD (4-bit)
- Potentiometer, wired as voltage divider into PA0
- Onboard LD2 LED, ST-Link USB

## Pin Table

| Signal | Pin | Notes |
|---|---|---|
| UART2 TX/RX | PA2 / PA3 | ST-Link VCP, 115200 8N1 |
| ADC (pot) | PA0 | ADC1_IN0, 12-bit |
| I2C1 SCL / SDA | PB8 / PB9 | 100 kHz |
| LCD RS | PA10 | |
| LCD EN | PB3 | |
| LCD D4–D7 | PB5, PB4, PB10, PA8 | |
| LD2 | PA5 | Toggled on OK→ALERT edge, also shell-controllable |
| B1 | PC13 | Configured, unused (no ISR attached) |

LCD needs a contrast pot/resistor on V0. BMP280 runs off 3.3V.

## Build

```bash
brew install --cask gcc-arm-embedded
brew install open-ocd
make
```

Produces `build/environmental-monitor.{elf,hex,bin}`. `make clean` to reset.

## Flash

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/environmental-monitor.elf verify reset exit"
```

Serial terminal:

```bash
ls /dev/tty.usbmodem*
screen /dev/tty.usbmodem14103 115200
```

## Shell Commands

| Command | Behavior |
|---|---|
| `HI` | Replies `HELLO` |
| `LED ON` / `LED OFF` | Drives LD2 |
| `READ_ADC` | Raw 12-bit ADC value |
| `READ_BMP` | Runs full `Sensors_Sample_And_Report()`: ADC + BMP280 + FSM + LCD update |
| `DEBUG_BMP` | Raw `adc_T`/`adc_P` plus `ctrl_meas`/`status` register readback |
| anything else | `Unknown command` |

## Sample Output

```
BMP280 init OK
UART ready
> READ_BMP
Temp: 24.13 C, Pressure: 1013.42 hPa, ADC: 1820, Threshold: 21.44 C, State: ALERT
*** ALERT: Temp 24.13 C >= threshold 21.44 C ***
> DEBUG_BMP
adc_T=419328 adc_P=349184 | ctrl_meas=0x27 status=0x00
```

Same report line is also emitted automatically once per second.

## FSM

- Any state → `SENSOR_ERROR` if the BMP280 read fails
- `OK` → `ALERT` when `temp_c >= threshold_c`
- `ALERT` → `OK` only when `temp_c < threshold_c - 1.0°C` (hysteresis)
- `SENSOR_ERROR` → re-evaluated fresh once the sensor recovers

Threshold = linear map of ADC (0–4095) onto 15–35°C.

## Design Decisions

**Interrupt-driven UART RX.** Main loop is busy with sampling, I2C, and LCD writes; polling for keystrokes would block it. RX interrupt captures bytes into a buffer and sets a flag, so the shell only costs time when a full line has arrived.

**Write-verified I2C writes.** `CTRL_MEAS`/`CONFIG` writes are read back and retried up to 5x. Without this, a dropped I2C write leaves the BMP280 in sleep mode with no obvious symptom — every read after that would return stale data silently. Verification turns that into an explicit init failure.

**Hysteresis on ALERT→OK.** Threshold is a linear ADC dial, so temperature sitting near it is common — without a hysteresis band it would flicker `OK`/`ALERT` on sensor noise.

**Edge-triggered LED.** Toggles once on the `OK→ALERT` transition rather than staying lit for the whole ALERT state, so it acts as a change notification instead of duplicating the LCD.

## Known Limitations

- `READ_BMP` returns the full sample report, not an isolated BMP-only reading
- B1 button is configured but has no ISR
- I2C/LCD writes are blocking — fine at 1 Hz, would need rework at higher rates
- No persistent alert history; state resets on reboot

## Repository Layout

```
Core/Inc/     bmp280.h, lcd.h, main.h, timer.h, stm32f4xx_it.h
Core/Src/
  main.c        shell, ADC, FSM, Sensors_Sample_And_Report()
  bmp280.c      BMP280 driver
  lcd.c         bit-banged 4-bit HD44780 driver
  timer.c       TIM2 1 Hz interrupt
  stm32f4xx_it.c  interrupt handlers
Drivers/      STM32 HAL + CMSIS
Makefile
STM32F446XX_FLASH.ld
startup_stm32f446xx.s
```