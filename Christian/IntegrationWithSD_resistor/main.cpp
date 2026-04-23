#include <avr/io.h>

#include "TFTdriver.h"
#include "ConsoleContext.h"
#include "MenuState.h"
#include "ConsoleInput.h"
#include "nrf24.h"
#include "controller_packet.h"

#include <stdlib.h>

int main(void)
{
	const uint8_t addr[5] = { 'N','R','F','2','4' };

	DDRB |= (1 << PB0);     // SD_CS / SS as output
	PORTB |= (1 << PB0);    // SD OFF

	DDRL |= (1 << PL6) | (1 << PL7);
	PORTL |= (1 << PL6);    // nRF CSN HIGH = inactive
	PORTL &= ~(1 << PL7);   // nRF CE LOW

	TFTDisplayInit();

	ConsoleContext context;
	context.recentScoreCount = 0;

	NRF24Init();
	NRF24InitRX(40, sizeof(ControllerPacket), addr);

	ConsoleInput_Init();

	srand(1234);

	context.SetState(&MenuState::Instance());

	while (1) {
		ConsoleInput_Update();
		context.Request();
	}
}