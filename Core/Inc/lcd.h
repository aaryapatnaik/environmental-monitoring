#ifndef LCD_H
#define LCD_H
 
#include <stdint.h>
 
// avoids pins used by adc (pa0), i2c1 (pb8/pb9), usart2 (pa2/pa3), onboard led (pa5)
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
 
// delay loop in lcd.c is timed off this, a wrong value throws off all lcd timing
#define LCD_SYSCLK_HZ   84000000UL
 
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char *str);
void LCD_PrintLine(uint8_t row, const char *str);
 
#endif