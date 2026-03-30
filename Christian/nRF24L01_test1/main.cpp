#include <avr/io.h>

#define F_CPU 16000000
#include <util/delay.h>

#include "nrf24.h"
#include "uart.h"

void UART_SendHex(uint8_t val)
{
	const char hex[] = "0123456789ABCDEF";
	UART_SendChar(hex[(val >> 4) & 0x0F]);
	UART_SendChar(hex[val & 0x0F]);
}

// TX-CODE

int main(void)
{
	const uint8_t addr[5] = { 'N','R','F','2','4' };
	uint8_t payload[4] = {1,2,3,4};

	UART_Init();

	UART_SendString("TX START\r\n");

	NRF24Init();
	NRF24InitTX(40, 4, addr);

	while (1)
	{
		UART_SendString("Sending: ");
		UART_SendHex(payload[0]);
		UART_SendString("\r\n");

		if (NRF24SendPacket(payload, 4)) {
			UART_SendString("OK\r\n");
			payload[0]++;
			} else {
			UART_SendString("FAIL\r\n");
		}

		_delay_ms(500);
	}
}
*/

// RX-CODE
/*
int main(void)
{
	const uint8_t addr[5] = { 'N','R','F','2','4' };
	uint8_t payload[4];

	UART_Init();

	UART_SendString("RX START\r\n");

	NRF24Init();
	NRF24InitRX(40, 4, addr);

	while (1)
	{
		if (NRF24DataReady()) {
			NRF24ReceivePacket(payload, 4);

			UART_SendString("RX: ");
			UART_SendHex(payload[0]);
			UART_SendString("\r\n");
		}
	}
}
*/