/*
 * usart.c
 *
 *  Created on: Mar 26, 2025
 *      Author: antonin
 */

#include "usart.h"
#include "stm32f0xx.h"

void USART_Init(void)
{
	// Enable GPIOA clock
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// Set PA2 (USART2_TX) and PA3 (USART2_RX) as AF1
	GPIOA->MODER &= ~(GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER3_Msk);
	GPIOA->MODER |= (0x02 << GPIO_MODER_MODER2_Pos) | (0x02 << GPIO_MODER_MODER3_Pos);
	GPIOA->AFR[0] &= ~(GPIO_AFRL_AFRL2_Msk | GPIO_AFRL_AFRL3_Msk);
	GPIOA->AFR[0] |= (0x01 << GPIO_AFRL_AFRL2_Pos) | (0x01 << GPIO_AFRL_AFRL3_Pos);

	// Use PCLK - 48MHz as USART2 peripheral clock (default)
	RCC->CFGR3 &= ~RCC_CFGR3_USART2SW_Msk;

	// Enable USART2 clock
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

	// Reset USART2 configuration - 8n1 mode - no parity control
	USART2->CR1 = 0x00000000;
	USART2->CR2 = 0x0000;
	USART2->CR3 = 0x0000;

	// Set baud rate for 38400 baud/s
	// BRR = f(PCLK)/baud rate = 48MHz / 38400 = 1250
	USART2->BRR = (uint16_t) 1250;

	// Enable transmitter and receiver
	USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;

	// Enable USART2
	USART2->CR1 |= USART_CR1_UE;

}

inline void USART_Disable(void)
{
	USART2->CR1 &= ~USART_CR1_UE;
}
