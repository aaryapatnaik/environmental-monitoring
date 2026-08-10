#ifndef TIMER_H
#define TIMER_H
 
#include "stm32f4xx_hal.h"
 
extern TIM_HandleTypeDef htim2;
extern volatile uint8_t sample_ready; // flag so sensor reads run in main loop, not isr
 
// interrupt mode instead of polling so the main loop isn't blocked waiting on the timer
void Sample_Timer_Init(void);
 
#endif