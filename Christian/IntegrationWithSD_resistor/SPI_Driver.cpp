#include <avr/io.h>
#include "SPI_Driver.h"

// nRF CSN on console
#define NRF_CSN_PORT PORTL
#define NRF_CSN_DDR  DDRL
#define NRF_CSN_BIT  6

// Deaktiver alle SPI devices
void SPI_Deactivate_All()
{
	PORTB |= (1 << SD_CS_BIT);          // SD OFF
	TFT_CS_PORT |= (1 << TFT_CS_BIT);   // TFT OFF
	NRF_CSN_PORT |= (1 << NRF_CSN_BIT); // nRF OFF
}

// SPI init
void SPI_init(void)
{
	// SPI pins
	SPI_DDR |= (1 << SD_CS_BIT);   // PB0
	SPI_DDR |= (1 << MOSI_BIT);
	SPI_DDR &= ~(1 << MISO_BIT);
	SPI_DDR |= (1 << SCK_BIT);

	// TFT CS pin
	TFT_CS_DDR |= (1 << TFT_CS_BIT);

	// nRF CS pin
	NRF_CSN_DDR |= (1 << NRF_CSN_BIT);

	// Slå ALT fra
	SPI_Deactivate_All();

	// SPI setup
	SPCR = 0b01010010;
	SPSR = 0;
}

unsigned char SPI_transmit(unsigned char data)
{
	SPDR = data;
	while (!(SPSR & (1 << SPIF))) {}
	return SPDR;
}

unsigned char SPI_receive()
{
	SPDR = 0xFF;
	while (!(SPSR & (1 << SPIF))) {}
	return SPDR;
}

// SD select
void SPI_Chip_Select()
{
	SPI_Deactivate_All();
	PORTB &= ~(1 << SD_CS_BIT); // SD ON
}

// SD deselect
void SPI_Chip_Deselect()
{
	SPI_Deactivate_All();
}