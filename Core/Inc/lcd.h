// driver for a 16x2 HD44780 LCD wired up in raw 4-bit parallel mode
// (no i2c backpack, just direct GPIO)

#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// pin mapping (nucleo-f446re arduino header)
// chosen to avoid pins already used by adc (pa0), i2c1 (pb8/pb9),
// usart2 (pa2/pa3), and the onboard led (commonly pa5)
//
//   LCD RS -> PA10 (D2)
//   LCD EN -> PB3  (D3)
//   LCD D4 -> PB5  (D4)
//   LCD D5 -> PB4  (D5)
//   LCD D6 -> PB10 (D6)
//   LCD D7 -> PA8  (D7)
//   LCD RW -> tie directly to GND (driver only ever writes)
//   LCD V0 -> wiper of a contrast trim pot (other legs to GND/VDD)
//   LCD VSS -> GND, VDD -> 5V, A -> 5V (backlight +), K -> GND (backlight -)

// set this to the actual core clock in hz, the delay loop in lcd.c is
// timed off it. this project runs the pll up to 84 MHz - if you run it
// bare-metal with no clock config the default is 16 MHz (HSI) instead
#define LCD_SYSCLK_HZ   84000000UL

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char *str);
void LCD_PrintLine(uint8_t row, const char *str);

#endif