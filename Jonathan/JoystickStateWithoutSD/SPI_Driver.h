#ifndef SPI_DRIVER_H_
#define SPI_DRIVER_H_

#include <avr/io.h>

#define SPI_PORT PORTB
#define SPI_DDR  DDRB

#define SD_CS_BIT   PB0
#define SCK_BIT     PB1
#define MOSI_BIT    PB2
#define MISO_BIT    PB3

// TFT CS er på PORTG bit 1
#define TFT_CS_PORT PORTG
#define TFT_CS_DDR  DDRG
#define TFT_CS_BIT  1

void SPI_init();
unsigned char SPI_transmit(unsigned char data);
unsigned char SPI_receive();

// SD control
void SPI_Chip_Select();
void SPI_Chip_Deselect();

// Shared control
void SPI_Deactivate_All();

#endif