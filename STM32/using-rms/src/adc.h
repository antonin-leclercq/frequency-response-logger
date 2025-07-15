/*
 * adc.h
 *
 *  Created on: Mar 12, 2025
 *      Author: antonin
 */

#ifndef SRC_ADC_H_
#define SRC_ADC_H_


#define ADC_buffer_size 1024

void ADC_DMA_Init(void);
void ADC_DMA_Enable(void);
void ADC_DMA_Disable(void);
void ADC_DMA_Interrupts_Enable(void);
uint16_t ADC_DMA_Get_Calibration_Factor(void);

enum dma_state
{
	RUNNING,
	FULL_TRANSFER,
};

#endif /* SRC_ADC_H_ */
