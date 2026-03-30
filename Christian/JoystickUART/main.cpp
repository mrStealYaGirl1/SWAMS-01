#include <avr/io.h>
#include <stdlib.h>  // Enabling itoa()
#define F_CPU 16000000
#include <util/delay.h>
#include <stdio.h>
#include "uart0.h"
#include "Joydriver.h"

// Lille helper til at sende printf-lignende uden stdio wiring
static void uart0_print_u16(const char *label, uint16_t v)
{
	char buf[8];
	uart0_puts(label);
	itoa((int)v, buf, 10);
	uart0_puts(buf);
}

int main(void)
{
	uart0_init(115200);
	JoyInit();

	uart0_puts("\r\nKY-23 test (X,Y,SW)\r\n");

	while (1)
	{
		JoyRaw jr;
		JoyReadRaw(&jr);

		uart0_print_u16("X=", jr.x);
		uart0_puts("  ");
		uart0_print_u16("Y=", jr.y);
		uart0_puts("  SW=");
		uart0_puts(jr.sw ? "1" : "0");
		uart0_puts("\r\n");

		_delay_ms(100);
	}
}