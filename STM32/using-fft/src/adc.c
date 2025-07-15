/*
 * adc.c
 *
 *  Created on: Mar 12, 2025
 *      Author: antonin
 */


#include "stm32f0xx.h"
#include "adc.h"

uint16_t adc_calibration_factor = 0;
int16_t adc_buffer[ADC_buffer_size] = {0};

uint32_t is_peripheral_enabled = 0;

void ADC_DMA_Init(void)
{
	// Using ADC channel 8 mapped on pin PB0

	// Using DMA requests for data transfer (peripheral to memory)
	// ADC mapped by default on DMA channel 1

	// ADC conversion triggered by TRGO from TIM3
	// PCLK frequency: 48MHz -> ADC maximum frequency: 24MHz

	// Enable GPIOB clock
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

	// Set PB0 as analog mode
	GPIOB->MODER &= ~GPIO_MODER_MODER0_Msk;
	GPIOB->MODER |= (0x03 << GPIO_MODER_MODER0_Pos);

	// Enable ADC clock
	RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

	// Reset ADC configuration
	ADC1->CR = 0x00000000;
	ADC1->CFGR1 = 0x00000000;
	ADC1->CFGR2 = 0x00000000;

	// Start calibration and wait for process to complete
	ADC1->CR |= ADC_CR_ADCAL;
	while((ADC1->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL);

	// Save calibration factor
	adc_calibration_factor = ADC1->DR;

	// Enable external triggering on rising edge
	ADC1->CFGR1 |= (0x01 << ADC_CFGR1_EXTEN_Pos);

	// Enable triggering on TRG3 (TIM3_TRGO)
	ADC1->CFGR1 |= (0x03 << ADC_CFGR1_EXTSEL_Pos);

	// Left align data for larger values for Q15 RFFT
	// ADC1->CFGR1 |= ADC_CFGR1_ALIGN;

	// Use PCLK/2 as input clock (24MHz)
	ADC1->CFGR2 |= (0x01 << ADC_CFGR2_CKMODE_Pos);

	// Select 28.5 cycles sampling time
	ADC1->SMPR = (uint16_t) (0x03 << ADC_SMPR_SMP_Pos);

	// Select channel 8 for conversion
	ADC1->CHSELR = (uint16_t) (0x01 << ADC_CHSELR_CHSEL8_Pos);

	//------------------------------------- DMA configuration -------------------------------------//

	// Enable DMA clock
	RCC->AHBENR |= RCC_AHBENR_DMAEN;

	// Reset DMA channel 1
	DMA1_Channel1->CCR = 0x00000000;

	// Set priority level to medium
	DMA1_Channel1->CCR |= (0x01 << DMA_CCR_PL_Pos);

	// Set memory and peripheral size to 16 bits
	DMA1_Channel1->CCR |= (0x01 << DMA_CCR_MSIZE_Pos) | (0x01 << DMA_CCR_PSIZE_Pos);

	// Enable memory increment mode
	DMA1_Channel1->CCR |= DMA_CCR_MINC;

	// Enable circular mode
	DMA1_Channel1->CCR |= DMA_CCR_CIRC;

	// Set number of data to transfer
	DMA1_Channel1->CNDTR = (uint16_t) ADC_buffer_size;

	// Set peripheral address
	DMA1_Channel1->CPAR = (uint32_t) &ADC1->DR;

	// Set memory address
	DMA1_Channel1->CMAR = (uint32_t) adc_buffer;

	// Enable DMA1 half transfer and full transfer interrupts
	DMA1_Channel1->CCR |= DMA_CCR_TCIE;

	is_peripheral_enabled = 0;

	//--------------------------------------------------------------------------------------------//
}

inline void ADC_DMA_Enable(void)
{
	if (is_peripheral_enabled == 0)
	{
		// Set number of data to transfer
		DMA1_Channel1->CNDTR = (uint16_t) ADC_buffer_size;

		// Enable DMA1 channel 1
		DMA1_Channel1->CCR |= DMA_CCR_EN;

		// Enable DMA requests
		ADC1->CFGR1 |= ADC_CFGR1_DMAEN;

		// Enable ADC
		ADC1->CR |= ADC_CR_ADEN;

		// Start conversion on trigger
		ADC1->CR |= ADC_CR_ADSTART;

		is_peripheral_enabled = 1;
	}
}

inline void ADC_DMA_Disable(void)
{
	if (is_peripheral_enabled == 1)
	{
		// Disable ADC
		ADC1->CR |= ADC_CR_ADDIS;
		while((ADC1->CR & ADC_CR_ADDIS) == ADC_CR_ADDIS);

		// Disable DMA
		DMA1_Channel1->CCR &= ~DMA_CCR_EN;

		is_peripheral_enabled = 0;
	}
}

inline void ADC_DMA_Interrupts_Enable(void)
{
	NVIC_SetPriority(DMA1_Channel1_IRQn, 16);
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

inline uint16_t ADC_DMA_Get_Calibration_Factor(void)
{
	return adc_calibration_factor;
}
