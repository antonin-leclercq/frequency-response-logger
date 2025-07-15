/*
 * timers.c
 *
 *  Created on: Mar 12, 2025
 *      Author: antonin
 */

#include "timers.h"

static uint32_t is_timer_enabled = 0;

void ACQ_Timer_Init(void)
{
	// Acquisition timer
	// Triggers ADC and DMA to copy at fixed sample rate
	// Using Timer 3, peripheral clock is 48MHz

	// Enable Timer 3 clock
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

	// Reset Timer 3 configuration
	TIM3->CR1 = 0x0000;
	TIM3->CR2 = 0x0000;

	// Enable ARR register buffering
	TIM3->CR1 |= TIM_CR1_ARPE;

	// Counter update used as TRGO
	TIM3->CR2 |= (0x02 << TIM_CR2_MMS_Pos);

	// Set pre-scaler for counter clock (1MHz counting - 1µs resolution)
	TIM3->PSC = (uint16_t) 48 -1;

	// Set auto-reload value
	TIM3->ARR = (uint16_t) 0xFFFF;

	is_timer_enabled = 0;
}

inline void ACQ_Timer_Set_Sampling_Period(uint16_t target_period_us)
{
	TIM3->ARR = target_period_us;
}

inline void ACQ_Timer_Disable(void)
{
	if (is_timer_enabled == 1)
	{
		TIM3->CR1 &= ~TIM_CR1_CEN;

		// Reinitialize TIM3
		TIM3->EGR |= TIM_EGR_UG;

		is_timer_enabled = 0;
	}
}

inline void ACQ_Timer_Enable(void)
{
	if (is_timer_enabled == 0)
	{
		TIM3->CR1 |= TIM_CR1_CEN;
		is_timer_enabled = 1;
	}
}
