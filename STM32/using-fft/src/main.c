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

// For FFT
#include "arm_math.h"
#include "arm_const_structs.h"

// Source of FFT
extern int16_t adc_buffer[ADC_buffer_size];

// Destination of FFT
#define FFT_LEN ADC_buffer_size
#define FFT_OUTPUT_LEN (FFT_LEN*2)

#define AVERAGING_LENGTH 10

int main(void)
{
	SystemClock_Config48MHz();
	USART_Init();
	stm32_printf("Program fft_example_f072\r\nSystem frequency: %u Hz\r\n", SystemCoreClock);

	uint16_t sampling_period = 200; // in microseconds
	ACQ_Timer_Init();
	ACQ_Timer_Set_Sampling_Period(sampling_period); // Set for 5 kHz sampling frequency
	ACQ_Timer_Enable();
	stm32_printf("Timer initialization complete\r\n");

	ADC_DMA_Init();
	stm32_printf("ADC initialization complete (calibration factor: %d)\r\n", ADC_DMA_Get_Calibration_Factor());

	arm_rfft_instance_q15 rfft = { 0 };
	int16_t fft_output[FFT_OUTPUT_LEN] = { 0 };

	// For magnitude of complex fft_output
	uint32_t fft_magnitude[FFT_LEN] = { 0 };

	// Initialize RFFT for forward transform
	if (arm_rfft_init_q15(&rfft, FFT_LEN, 0, 1) != ARM_MATH_SUCCESS)
	{
		stm32_printf("[ERROR] arm_rfft_init_q15 FAIL\r\nConsider resetting the MCU\r\n");
		ACQ_Timer_Disable();
		ADC_DMA_Disable();
		USART_Disable();
		while(1) {}
	}

	float hann_window[FFT_LEN] = { 0 };
	generate_hann_window(hann_window, FFT_LEN);

	stm32_printf("ARM RFFT initialization complete\r\n");

	// Enable ADC, DMA interrupts
	ADC_DMA_Interrupts_Enable();

	float average_frequency = 0.f;
	float average_magnitude = 0.f;
	uint32_t average_counter = 0;

	while(1)
	{
		// Receive from computer UART the expected frequency and adjust sampling frequency
		// Start ADC and DMA
		// When DMA FULL TRANSFER done, stop ADC and DMA + do FFT on samples + set sampling frequency to be roughly 3 times the signal frequency
		// Send FFT result to computer

		ACQ_Timer_Enable();
		ADC_DMA_Enable();

		if (dma_channel1_state == FULL_TRANSFER)
		{
			ADC_DMA_Disable();
			ACQ_Timer_Disable();

			// Shift adc_buffer by 3 to the left for larger values for RFFT
			// Apply window
			for (uint32_t i = 0; i < ADC_buffer_size; ++i)
			{
				adc_buffer[i] <<= 3;
				adc_buffer[i] = (uint16_t)((float)adc_buffer[i] * hann_window[i]);
			}

			// Execute RFFT (input: adc_buffer, output: fft_output)
			// Input format is Q1.15, output format is Q10.6, in complex numbers
			// First element is DC offset, then first half is positive frequency and second half is negative frequency
			arm_rfft_q15(&rfft, adc_buffer, fft_output);

			// Convert complex data to magnitude
			// Skipping first two elements (DC offset only)
			// Skipping first four elements with Hann windowing
			// Skipping second half of data since it's the complex conjugate of the first half
			for (uint32_t i = 0; i < (FFT_LEN /2) -1; ++i)
			{
				uint32_t j = (i << 1) + 4;
				fft_magnitude[i] = (fft_output[j]*fft_output[j] + fft_output[j+1]*fft_output[j+1]) / (float)FFT_LEN;
			}

			uint32_t fft_max = 0;
			uint32_t fft_max_index = 0;
			fft_find_max(fft_magnitude, 0, (FFT_LEN /2) -1, &fft_max, &fft_max_index);

			// frequency = index / FFT_LEN * sampling frequency
			float max_frequency = (float)fft_max_index * 1000000.f / (sampling_period * FFT_LEN);

			average_frequency += (1.f/AVERAGING_LENGTH) * max_frequency;
			average_magnitude += (1.f/AVERAGING_LENGTH) * fft_max;

			if (average_counter == AVERAGING_LENGTH)
			{
				average_counter = 0;
				sampling_period = (0.25f * (1000000.f/(average_frequency + 100)));
				stm32_printf("Updated Ts: %d, Peak frequency: %d Hz, Peak: %d     \r", sampling_period, (uint32_t)average_frequency, (uint32_t)average_magnitude);
				average_frequency = 0.f;
				average_magnitude = 0.f;
				ACQ_Timer_Set_Sampling_Period(sampling_period);
			}

			// print_fft(fft_output, FFT_OUTPUT_LEN);

			dma_channel1_state = RUNNING;
			++average_counter;
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

void print_fft(const int16_t* fft_result, const uint32_t fft_size)
{
	stm32_printf("\r\nFFT Results raw data (actual format: Q10.6) :\r\n [");
	for (uint32_t i = 0; i < fft_size; ++i)
	{
		stm32_printf("%d, ", fft_result[i]);
	}
	stm32_printf("\r\n");
}

void print_magnitude(const uint32_t* fft_mag, const uint32_t fft_size)
{
	stm32_printf("\r\nFFT Results Magnitude :\r\n [");
	for (uint32_t i = 0; i < fft_size; ++i)
	{
		stm32_printf("%d, ", fft_mag[i]);
	}
	stm32_printf("\r\n");
}

void fft_find_max(const uint32_t* fft_magnitude, const uint32_t fft_start, const uint32_t fft_end,
		uint32_t* fft_max, uint32_t* fft_max_index)
{
	*fft_max = 0;
	*fft_max_index = fft_start;
	for (uint32_t i = fft_start; i < fft_end; ++i)
	{
		if (*fft_max < fft_magnitude[i])
		{
			*fft_max_index = i;
			*fft_max = fft_magnitude[i];
		}
	}
}

void generate_hann_window(float* hann_buffer, const uint32_t hann_size)
{
	for (uint32_t i = 0; i < hann_size; ++i)
	{
		hann_buffer[i] = 0.5f - 0.5f*arm_cos_f32(6.283185307179586f * i / (hann_size -1));
	}
}
