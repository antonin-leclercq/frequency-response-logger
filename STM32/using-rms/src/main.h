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
uint16_t calculate_sampling_period(const char* usart_buffer, const uint32_t n_digits);
void clear_buffer(char* buffer, const uint32_t size);
#endif /* SRC_MAIN_H_ */
