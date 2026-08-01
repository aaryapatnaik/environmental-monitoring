#ifndef LCD_H
#define LCD_H
 
#include <stdint.h>
 
/* ---- Pin mapping (Nucleo-F446RE Arduino header) ----
 * Adjust these if your wiring differs. Chosen to avoid pins already
 * used by ADC (PA0), I2C1 (PB8/PB9), USART2 (PA2/PA3), and the
 * onboard LED (commonly PA5).
 *
 *   LCD RS -> PA10 (D2)
 *   LCD EN -> PB3  (D3)
 *   LCD D4 -> PB5  (D4)
 *   LCD D5 -> PB4  (D5)
 *   LCD D6 -> PB10 (D6)
 *   LCD D7 -> PA8  (D7)
 *   LCD RW -> tie directly to GND (driver only ever writes)
 *   LCD V0 -> wiper of a contrast trim pot (other legs to GND/VDD)
 *   LCD VSS -> GND, VDD -> 5V, A -> 5V (backlight +), K -> GND (backlight -)
 */
 
/* Set this to your ACTUAL core clock in Hz. Bare-metal default on the
 * F446RE with no clock config is 16 MHz (HSI). If you've set up the
 * PLL for a higher SYSCLK, change this to match, or the timed portions
 * of the init sequence will be off. */
#define LCD_SYSCLK_HZ   84000000UL
 
void LCD_Init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(const char *str);
void lcd_print_line(uint8_t row, const char *str);
 
#endif /* LCD_H */