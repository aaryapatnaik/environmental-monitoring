#ifndef TIMER_H
#define TIMER_H

#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim2;
extern volatile uint8_t sample_ready;

void Sample_Timer_Init(void);

#endif