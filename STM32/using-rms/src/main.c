/*
 * main.c
 *
 *  Created on: Feb 7, 2025
 *      Author: antonin
 */

#include "main.h"
#include "timers.h"
#include "adc.h"
#include "usart.h"

extern int stm32_printf(const char *format, ...);
extern enum dma_state dma_channel1_state;

// Source of FFT
extern int16_t adc_buffer[ADC_buffer_size];

#define AVERAGING_LENGTH 10

#define CARRIAGE_RETURN 0x0D

int main(void)
{
	SystemClock_Config48MHz();
	USART_Init();
	stm32_printf("Program rms_calulator\r\nSystem frequency: %u Hz\r\n", SystemCoreClock);

	uint16_t sampling_period = 100; // in microseconds
	ACQ_Timer_Init();
	ACQ_Timer_Set_Sampling_Period(sampling_period); // Set for 10 kHz sampling frequency initially
	ACQ_Timer_Enable();
	stm32_printf("Timer initialization complete\r\n");

	ADC_DMA_Init();
	stm32_printf("ADC initialization complete (calibration factor: %d)\r\n", ADC_DMA_Get_Calibration_Factor());

	// Enable ADC, DMA interrupts
	ADC_DMA_Interrupts_Enable();

	float buffer_average_magnitude = 0.f;
	float average_magnitude = 0.f;
	uint32_t average_counter = 0;

	char usart_buffer[8] = { 0 };
	uint32_t usart_index = 0;
	char c = 0;
	uint32_t acq_peripherals_enabled = 0;

	// Status LED /////////////////////
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER &= ~GPIO_MODER_MODER5_Msk;
	GPIOA->MODER |= (0x01 << GPIO_MODER_MODER5_Pos);
	GPIOA->ODR &= ~GPIO_ODR_5;
	//////////////////////////////////

	while(1)
	{
		// Receive from computer UART the expected frequency and adjust sampling frequency
		// Start ADC and DMA
		// When DMA FULL TRANSFER done, stop ADC and DMA + do RMS calculation + set new sampling frequency
		// Send RMS result to computer

		if(acq_peripherals_enabled == 0 && average_counter == 0 && (USART2->ISR & USART_ISR_RXNE) == USART_ISR_RXNE)
		{
			c = (char) USART2->RDR;

			// echo back
			//while((USART2->ISR & USART_ISR_TXE) != USART_ISR_TXE);
			//USART2->TDR = c;

			if (c == CARRIAGE_RETURN)
			{
				// Update sampling frequency
				sampling_period = (uint16_t)(0.25f * (float)calculate_sampling_period(usart_buffer, usart_index));
				ACQ_Timer_Set_Sampling_Period(sampling_period);

				clear_buffer(usart_buffer, 8);
				usart_index = 0;

				ACQ_Timer_Enable();
				ADC_DMA_Enable();
				acq_peripherals_enabled = 1;

				GPIOA->ODR &= ~GPIO_ODR_5;
			}
			else if (usart_index < 8)
			{
				usart_buffer[usart_index++] = c;
			}
		}


		if (dma_channel1_state == FULL_TRANSFER)
		{
			ADC_DMA_Disable();
			ACQ_Timer_Disable();
			acq_peripherals_enabled = 0;

			GPIOA->ODR |= GPIO_ODR_5;

			for (uint32_t i = 0; i < ADC_buffer_size; ++i)
			{
				buffer_average_magnitude += (float) adc_buffer[i];
			}
			buffer_average_magnitude /= (float) ADC_buffer_size;

			average_magnitude += buffer_average_magnitude / (float) AVERAGING_LENGTH;

			if (average_counter >= AVERAGING_LENGTH-1)
			{
				average_counter = 0;
				stm32_printf("%d\n", (uint32_t) average_magnitude); // Send data to computer
				average_magnitude = 0.f;
			}
			else
			{
				// DO ACQ again for the average
				++average_counter;
				ACQ_Timer_Enable();
				ADC_DMA_Enable();
				acq_peripherals_enabled = 1;
				GPIOA->ODR &= ~GPIO_ODR_5;
			}
			dma_channel1_state = RUNNING;
		}
	}
	return 0;
}

void SystemClock_Config48MHz(void)
{
	// SYSCLK target frequency : 48MHz

	uint32_t timeout = 0;

	// Start HSE in Bypass Mode
	RCC->CR |= RCC_CR_HSEBYP;
	RCC->CR |= RCC_CR_HSEON;

	// Wait until HSE is ready
	while(((RCC->CR & RCC_CR_HSERDY) != RCC_CR_HSERDY) && (++timeout < 100000));

	// Select HSE as PLL input source
	RCC->CFGR &= ~RCC_CFGR_PLLSRC_Msk;
	RCC->CFGR |= (0x02 <<RCC_CFGR_PLLSRC_Pos);

	// Set PLL PREDIV to /1
	RCC->CFGR2 = 0x00000000;

	// Set PLL MUL to x2
	RCC->CFGR &= ~RCC_CFGR_PLLMUL_Msk;
	RCC->CFGR |= (0x04 << RCC_CFGR_PLLMUL_Pos);

	// Enable the main PLL
	RCC-> CR |= RCC_CR_PLLON;

	// Wait until PLL is ready
	timeout = 0;
	while(((RCC->CR & RCC_CR_PLLRDY) != RCC_CR_PLLRDY) && (++timeout < 100000));

	// Set AHB prescaler to /1
	RCC->CFGR &= ~RCC_CFGR_HPRE_Msk;

	//Set APB1 prescaler to /1
	RCC->CFGR &= ~RCC_CFGR_PPRE_Msk;

	// Enable FLASH Prefetch Buffer and set correct wait-states
	FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;

	// Select the main PLL as system clock source
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	// Wait until PLL becomes main switch input
	timeout = 0;
	while(((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) && (++timeout < 100000));

	// Update SystemCoreClock global variable
	SystemCoreClockUpdate();
}

uint16_t calculate_sampling_period(const char* usart_buffer, const uint32_t n_digits)
{
	uint16_t result = 0;
	uint16_t multiplier = 1;
	for (uint32_t i = 0; i < n_digits; ++i)
	{
		result += (usart_buffer[n_digits - i - 1] - '0') * multiplier;
		multiplier *= 10;
	}
	return result;
}

void clear_buffer(char* buffer, const uint32_t size)
{
	for (uint32_t i = 0; i < size; ++i)
	{
		buffer[i] = 0;
	}
}
