/*
 * main.h
 *
 *  Created on: Feb 7, 2025
 *      Author: antonin
 */

#include "stm32f0xx.h"

#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

void SystemClock_Config48MHz(void);
void print_fft(const int16_t* fft_result, const uint32_t fft_size);
void fft_find_max(const uint32_t* fft_magnitude, const uint32_t fft_start, const uint32_t fft_end,
		uint32_t* fft_max, uint32_t* fft_max_index);
void generate_hann_window(float* hann_buffer, const uint32_t hann_size);
void print_magnitude(const uint32_t* fft_mag, const uint32_t fft_size);

#endif /* SRC_MAIN_H_ */
