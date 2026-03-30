#include "uart0.h"
#include <avr/io.h>

#define F_CPU 16000000
#include <util/delay.h>

static uint16_t uart_ubrr(uint32_t baud)
{
	// Normal mode (U2X0=0): UBRR = F_CPU/(16*baud) - 1
	// Runder til nærmeste
	uint32_t ubrr = (F_CPU + (8UL * baud)) / (16UL * baud) - 1UL;
	return (uint16_t)ubrr;
}

void uart0_init(uint32_t baud)
{
	uint16_t ubrr = uart_ubrr(baud);

	// Baud
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)(ubrr);

	// 8N1
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

	// Enable TX (og RX hvis du vil)
	UCSR0B = (1 << TXEN0);
}

void uart0_putc(char c)
{
	while (!(UCSR0A & (1 << UDRE0))) { }
	UDR0 = (uint8_t)c;
}

void uart0_puts(const char *s)
{
	while (*s) uart0_putc(*s++);
}