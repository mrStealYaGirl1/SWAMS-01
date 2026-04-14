#include <avr/io.h>

#define F_CPU 16000000
#include <util/delay.h>

#include "nrf24.h"
#include "uart.h"
#include "Joydriver.h"
#include "controller_packet.h"

void UART_SendHex(uint8_t val)
{
	const char hex[] = "0123456789ABCDEF";
	UART_SendChar(hex[(val >> 4) & 0x0F]);
	UART_SendChar(hex[val & 0x0F]);
}

static int8_t JoyNormalizeAxis(uint16_t raw)
{
	int16_t centered = (int16_t)raw - 512;
	int16_t scaled = centered / 4;

	if (scaled > 127) scaled = 127;
	if (scaled < -127) scaled = -127;

	return (int8_t)scaled;
}


static void BuildControllerPacket(ControllerPacket *pkt)
{
	JoyRaw jr;

	pkt->joyX = 0;
	pkt->joyY = 0;
	pkt->buttons = 0;
	pkt->flags = 0;

	if (!JoyReadRaw(&jr)) {
		return;
	}

	pkt->joyX = JoyNormalizeAxis(jr.x);
	pkt->joyY = JoyNormalizeAxis(jr.y);

	if (jr.sw) {
		pkt->buttons |= CTRL_BTN_JOYSTICK;
	}

	pkt->flags |= CTRL_FLAG_VALID;
}

// TX-Code

int main(void)
{
	const uint8_t addr[5] = { 'N','R','F','2','4' };
	ControllerPacket pkt;

	UART_Init();
	JoyInit();

	UART_SendString("TX START\r\n");

	NRF24Init();
	NRF24InitTX(40, sizeof(ControllerPacket), addr);

	while (1)
	{
		BuildControllerPacket(&pkt);

		UART_SendString("TX X=");
		UART_SendHex((uint8_t)pkt.joyX);
		UART_SendString(" Y=");
		UART_SendHex((uint8_t)pkt.joyY);
		UART_SendString(" B=");
		UART_SendHex(pkt.buttons);
		UART_SendString("\r\n");

		if (NRF24SendPacket((const uint8_t*)&pkt, sizeof(ControllerPacket))) {
			UART_SendString("OK\r\n");
			} else {
			UART_SendString("FAIL\r\n");
		}

		_delay_ms(50);
	}
}

// RX-Code
/*
int main(void)
{
	const uint8_t addr[5] = { 'N','R','F','2','4' };
	ControllerPacket pkt;

	UART_Init();
	UART_SendString("RX START\r\n");

	NRF24Init();
	NRF24InitRX(40, sizeof(ControllerPacket), addr);

	while (1)
	{
		if (NRF24DataReady()) {
			NRF24ReceivePacket((uint8_t*)&pkt, sizeof(ControllerPacket));

			UART_SendString("RX X=");
			UART_SendHex((uint8_t)pkt.joyX);
			UART_SendString(" Y=");
			UART_SendHex((uint8_t)pkt.joyY);
			UART_SendString(" B=");
			UART_SendHex(pkt.buttons);
			UART_SendString(" F=");
			UART_SendHex(pkt.flags);
			UART_SendString("\r\n");
		}
	}
}
*/