/*
 * timers.h
 *
 *  Created on: Mar 12, 2025
 *      Author: antonin
 */

#ifndef SRC_TIMERS_H_
#define SRC_TIMERS_H_

#include "stm32f0xx.h"

void ACQ_Timer_Init(void);
void ACQ_Timer_Set_Sampling_Period(uint16_t target_period);
void ACQ_Timer_Disable(void);
void ACQ_Timer_Enable(void);

#endif /* SRC_TIMERS_H_ */
