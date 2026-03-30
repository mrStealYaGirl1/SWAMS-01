#include "uart.h"

#define F_CPU 16000000UL
#include <avr/io.h>

#define BAUD 9600
#define UBRR_VALUE ((F_CPU/16/BAUD)-1)

void UART_Init(void)
{
	UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
	UBRR0L = (uint8_t)UBRR_VALUE;

	UCSR0B = (1 << TXEN0);              // Enable TX
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit
}

void UART_SendChar(char c)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

void UART_SendString(const char *str)
{
	while (*str) {
		UART_SendChar(*str++);
	}
}