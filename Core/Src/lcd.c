#include "lcd.h"
#include "stm32f446xx.h"
 
/* ---------- pin definitions, derived from lcd.h wiring table ---------- */
#define LCD_RS_PORT   GPIOA
#define LCD_RS_PIN    10
 
#define LCD_EN_PORT   GPIOB
#define LCD_EN_PIN    3
 
#define LCD_D4_PORT   GPIOB
#define LCD_D4_PIN    5
 
#define LCD_D5_PORT   GPIOB
#define LCD_D5_PIN    4
 
#define LCD_D6_PORT   GPIOB
#define LCD_D6_PIN    10
 
#define LCD_D7_PORT   GPIOA
#define LCD_D7_PIN    8
 
/* HD44780 commands */
#define LCD_CLEARDISPLAY    0x01
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_FUNCTIONSET     0x20
#define LCD_SETDDRAMADDR    0x80
 
#define LCD_ENTRYLEFT       0x02
#define LCD_DISPLAYON       0x04
#define LCD_4BITMODE        0x00
#define LCD_2LINE           0x08
#define LCD_5x8DOTS         0x00
 
/* ---------------------------- timing ---------------------------- */
 
static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
 
static void lcd_delay_us(uint32_t us)
{
    uint32_t cycles = (LCD_SYSCLK_HZ / 1000000UL) * us;
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles) { }
}
 
static void lcd_delay_ms(uint32_t ms)
{
    while (ms--) {
        lcd_delay_us(1000);
    }
}
 
/* ---------------------------- GPIO helpers ---------------------------- */
 
static void gpio_out_init(GPIO_TypeDef *port, uint8_t pin)
{
    /* MODER: 01 = general purpose output */
    port->MODER &= ~(0x3U << (pin * 2));
    port->MODER |=  (0x1U << (pin * 2));
    /* push-pull, low speed, no pull is fine for these lines */
    port->OTYPER  &= ~(0x1U << pin);
    port->OSPEEDR &= ~(0x3U << (pin * 2));
    port->PUPDR   &= ~(0x3U << (pin * 2));
}
 
static inline void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t level)
{
    if (level) {
        port->BSRR = (1U << pin);
    } else {
        port->BSRR = (1U << (pin + 16));
    }
}
 
/* ---------------------------- low-level LCD I/O ---------------------------- */
 
static void lcd_pulse_enable(void)
{
    gpio_write(LCD_EN_PORT, LCD_EN_PIN, 0);
    lcd_delay_us(1);
    gpio_write(LCD_EN_PORT, LCD_EN_PIN, 1);
    lcd_delay_us(1);   /* EN pulse width > 450ns */
    gpio_write(LCD_EN_PORT, LCD_EN_PIN, 0);
    lcd_delay_us(100); /* command settle time */
}
 
static void lcd_write4(uint8_t nibble)
{
    gpio_write(LCD_D4_PORT, LCD_D4_PIN, (nibble >> 0) & 0x1);
    gpio_write(LCD_D5_PORT, LCD_D5_PIN, (nibble >> 1) & 0x1);
    gpio_write(LCD_D6_PORT, LCD_D6_PIN, (nibble >> 2) & 0x1);
    gpio_write(LCD_D7_PORT, LCD_D7_PIN, (nibble >> 3) & 0x1);
    lcd_pulse_enable();
}
 
static void lcd_send(uint8_t value, uint8_t is_data)
{
    gpio_write(LCD_RS_PORT, LCD_RS_PIN, is_data ? 1 : 0);
    lcd_write4(value >> 4);
    lcd_write4(value & 0x0F);
}
 
static void lcd_send_cmd(uint8_t cmd)
{
    lcd_send(cmd, 0);
    if (cmd == LCD_CLEARDISPLAY || cmd == 0x02 /* return home */) {
        lcd_delay_ms(2); /* these two need much longer than other commands */
    }
}
 
static void lcd_send_data(uint8_t data)
{
    lcd_send(data, 1);
}
 
void LCD_Init(void)
{
    dwt_init();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
 
    gpio_out_init(LCD_RS_PORT, LCD_RS_PIN);
    gpio_out_init(LCD_EN_PORT, LCD_EN_PIN);
    gpio_out_init(LCD_D4_PORT, LCD_D4_PIN);
    gpio_out_init(LCD_D5_PORT, LCD_D5_PIN);
    gpio_out_init(LCD_D6_PORT, LCD_D6_PIN);
    gpio_out_init(LCD_D7_PORT, LCD_D7_PIN);
 
    gpio_write(LCD_RS_PORT, LCD_RS_PIN, 0);
    gpio_write(LCD_EN_PORT, LCD_EN_PIN, 0);
 
    lcd_delay_ms(40); /* power-on settle, HD44780 needs >15ms at 4.5V/>40ms at 2.7V */
 
    /* HD44780 4-bit init sequence: three 8-bit "0x3" nibbles, spaced out,
     * then switch to 4-bit mode. This has to be done exactly this way -
     * it's how the controller detects it's talking to a 4-bit bus. */
    lcd_write4(0x03);
    lcd_delay_ms(5);
    lcd_write4(0x03);
    lcd_delay_us(150);
    lcd_write4(0x03);
    lcd_delay_us(150);
    lcd_write4(0x02); /* now actually enter 4-bit mode */
 
    lcd_send_cmd(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);
    lcd_send_cmd(LCD_DISPLAYCONTROL | LCD_DISPLAYON); /* display on, cursor/blink off */
    lcd_send_cmd(LCD_CLEARDISPLAY);
    lcd_send_cmd(LCD_ENTRYMODESET | LCD_ENTRYLEFT);   /* auto-increment cursor */
}
 
void lcd_clear(void)
{
    lcd_send_cmd(LCD_CLEARDISPLAY);
}
 
void lcd_set_cursor(uint8_t row, uint8_t col)
{
    static const uint8_t row_offsets[2] = {0x00, 0x40};
    if (row > 1) row = 1;
    if (col > 15) col = 15;
    lcd_send_cmd(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}
 
void lcd_print(const char *str)
{
    while (*str) {
        lcd_send_data((uint8_t)*str++);
    }
}

void lcd_print_line(uint8_t row, const char *str)
{
    lcd_set_cursor(row, 0);
    uint8_t col = 0;
    while (*str && col < 16) {
        lcd_send_data((uint8_t)*str++);
        col++;
    }
    while (col < 16) {
        lcd_send_data(' ');
        col++;
    }
}