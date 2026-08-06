// tim2 config for 1 hz periodic sampling. isr sets sample_ready, main loop
// polls the flag and clears it after handling a sample (see main.c)
 
#ifndef TIMER_H
#define TIMER_H
 
#include "stm32f4xx_hal.h"
 
extern TIM_HandleTypeDef htim2;
extern volatile uint8_t sample_ready; // set by the tim2 isr, cleared by main loop
 
// sets up tim2 and starts it in interrupt mode
void Sample_Timer_Init(void);
 
#endif