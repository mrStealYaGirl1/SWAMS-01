/************************************************************
File name: "nrf24.cpp"

Minimal driver for nRF24L01 on ATmega2560.
Version 1: fixed payload, no auto-ack, no retransmit,
polling-based TX/RX.

Intended for:
- understanding the datasheet
- testing register access over SPI
- testing TX/RX between two MCUs

Max MCU clock = 16 MHz

ATmega2560 hardware SPI:
MOSI: PB2
MISO: PB3
SCK : PB1
SS  : PB0

Choose CE and CSN freely

************************************************************/

#include <avr/io.h>

#define F_CPU 16000000
#include <util/delay.h>

// #include <avr/cpufunc.h>  // _NOP()
// Ovenstående bruges ikke, selvom vi laver timing

#include "nrf24.h"

// PINS DEFINITIONS /////////////////////////////////////

// CE (Chip Enable Activates RX or TX mode)
#define NRF_CE_DDR DDRL
#define NRF_CE_PORT PORTL
#define NRF_CE_BIT 7

// CSN (SPI Chip Select)
#define NRF_CSN_DDR DDRL
#define NRF_CSN_PORT PORTL
#define NRF_CSN_BIT 6

// Dedicated SPI pins on ATMega2560
#define NRF_SPI_DDR DDRB
#define NRF_SPI_PORT PORTB
#define NRF_MOSI_BIT 2 // Page 2 from ATMega2560 databook
#define NRF_MISO_BIT 3
#define NRF_SCK_BIT 1
#define NRF_SS_BIT 0

// COMMAND SET /////////////////////////////////////////////////////////////////

#define NRF_CMD_R_REGISTER 0b00000000 // - R_REGISTER Command name 
#define NRF_CMD_W_REGISTER 0b00100000 // - W_REGISTER Command name
#define NRF_CMD_R_RX_PAYLOAD 0b01100001 // - R_RX_PAYLOAD Command name 
#define NRF_CMD_W_TX_PAYLOAD 0b10100000 // - W_TX_PAYLOAD Command name 
#define NRF_CMD_FLUSH_TX 0b11100001 // - FLUSH_TX Command name 
#define NRF_CMD_FLUSH_RX 0b11100010 // - FLUSH_RX Command name 
#define NRF_CMD_NOP 0b11111111 // - NOP Command name 

#define NRF_CMD_REGISTER_MAP_ADDRESS 0b00011111 // AAAAA = 5 bit Register Map Address

// REGISTER MAP ////////////////////////////////////////////////////////////////

#define NRF_REG_CONFIG 0x00
#define NRF_REG_EN_AA 0x01 
#define NRF_REG_EN_RXADDR 0x02
#define NRF_REG_SETUP_AW 0x03
#define NRF_REG_SETUP_RETR 0x04
#define NRF_REG_RF_CH 0x05
#define NRF_REG_RF_SETUP 0x06
#define NRF_REG_STATUS 0x07
#define NRF_REG_RX_ADDR_P0 0x0A // Receive address data pipe 0. 5 Bytes maximumlength. (LSByte is written first. Write the number of bytes defined by SETUP_AW)

// We only have one receiver, so we don't define the other data pipes

#define NRF_REG_TX_ADDR 0x10
#define NRF_REG_RX_PW_P0 0x11
#define NRF_REG_FIFO_STATUS 0x17

// BIT DEFINITIONS /////////////////////////////////////////////////////////////

//NRF_"RegisterMnemonic"_"BitMnemonic"

#define NRF_CONFIG_EN_CRC 3
#define NRF_CONFIG_CRCO 2
#define NRF_CONFIG_PWR_UP 1
#define NRF_CONFIG_PRIM_RX 0 

#define NRF_STATUS_RX_DR 6
#define NRF_STATUS_TX_DS 5
#define NRF_STATUS_MAX_RT 4

#define NRF_FIFO_RX_EMPTY 0

// LOCAL FUNCTIONS /////////////////////////////////////////////////////////////

static void NRF24CEHigh(void) // We define this, since this signal is active high and is used to activate the chip in RX or TX mode
{
	NRF_CE_PORT |= (1 << NRF_CE_BIT); // Behold alt - men sørg for at denne bit (NRF_CE_BIT) er 1 for CE-port'en (Port L)
}

static void NRF24CELow(void) // We define this, since this signal is active high and is used to activate the chip in RX or TX mode
{
	NRF_CE_PORT &= ~(1 << NRF_CE_BIT); // Behold alt – men sørg for at denne bit (NRF_CE_BIT) er 0 for CE-port'en (Port L)
}

static void NRF24CSNHigh(void) // We define this, since this initiates either SPI read og write operation
{
	NRF_CSN_PORT |= (1 << NRF_CSN_BIT); // Behold alt - men sørg for at denne bit (NRF_CSN_BIT) er 1 for CSN-port'en (Port L)
}

static void NRF24CSNLow(void)
{
	NRF_CSN_PORT &= ~(1 << NRF_CSN_BIT); // Behold alt – men sørg for at denne bit (NRF_CSN_BIT) er 0 for CSN-port'en (Port L)
}

static void NRF24SPIInit(void)
{
	// "Set MOSI, SS and SCK output, all others input" (Reference: ATMega2560 DataBook, page 193, C Code Example)
	
	// NRF_SPI_DDR = (1 << NRF_MOSI_BIT) | (1 << NRF_SCK_BIT) | (1 << NRF_SS_BIT);
	
	// Bedre implementering er at vi ikke overskriver hele DDRB
	NRF_SPI_DDR |= (1 << NRF_MOSI_BIT) | (1 << NRF_SCK_BIT) | (1 << NRF_SS_BIT);
	NRF_SPI_DDR &= ~(1 << NRF_MISO_BIT);
	
	// "Enable SPI, Master, set clock rate fck/16" (Reference: ATMega2560 DataBook, page 193, C Code Example)
	// SPE: SPI Enable - "When the SPE bit is written to one, the SPI is enabled. This bit must be set to enable any SPI operations." - (Reference: ATMega2560 DataBook, page 197, SPCR Bit 6)
	// MSTR: Master/Slave Select - "This bit selects Master SPI mode when written to one, and Slave SPI mode when written logic zero." - (Reference: ATMega2560 DataBook, page 197, SPCR Bit 4)
	// Bits 1, 0 – SPR1, SPR0: SPI Clock Rate Select 1 and 0 - "These two bits control the SCK rate of the device configured as a Master." - (Reference: ATMega2560 DataBook, page 198, SPCR Bits 1, 0)
		// Kombination ved at enable SPR0 bit og disable SPR1 bit gør at vi får en SCK frekvens på fosc/ 16 --> 16 MHz / 16 = 1 MHz
		// SCK Frekvens på 1 MHz er fint som clock til nRF24L01
		
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); // SPCR - SPI Control Register
	
	// BEMÆRK AT CPOL BIT 3 OG CPHA BIT 2 ER BEGGE 0, FORDI AT NÅR MAN OBSERVERER TIMING DIAGRAMMET FOR nRF24L01, SÅ ER CPOL = 0 og CPHA = 0
	// ---> DERFOR SÆTTER VI DE TO BITS (3 og 2) TIL LOW
	
	// BEMÆRK AT PÅ TIMING DIAGRAMMET FOR nRF24L01, SÅ ER DET MSB FIRST. I Figure 21-3 i ATMega2560 databook'en, så sætter man DORD = 0, for at få MSB first
	// "DORD" er bit 5 i SPCR 
	SPSR = 0;
} 

static uint8_t NRF24SPITransceive(uint8_t data) // "8 bit command set" + "Command word: MSBit to LSBit (one byte)" + "You can use the SPI to … by 1 byte SPI commands..."
{
	/* Start transmission */
	SPDR = data;
	
	/* Wait for transmission complete */
	/*while(!(SPSR & (1<<SPIF))) // FIND UD AF HVAD DET HER GØR
	{}
	return SPDR */
	
	while(!(SPSR & (1<<SPIF))) // FIND UD AF HVAD DET HER GØR
	;
	
	// (Ovenstående er reference: ATMega2560 DataBook, page 193, C Code Example)
	
	return SPDR;
}

static void NRF24PinsInit(void)
{
	// Control pins are outputs
	
	NRF_CE_DDR  |= (1 << NRF_CE_BIT);
	NRF_CSN_DDR |= (1 << NRF_CSN_BIT);
	
	NRF24CELow(); // "No ongoing packet transmission" - Standby-I operational mode (Observer afsnit 6.1.6 omkring nRF24 main modes)
	// Vi starter med at der ikke udføres packet transmission --> De kalder det "Standby-I" mode.
	// CE har "PWR_UP" bitten tilfælles, for at forklare de modes, som nRF24 kan være i. Med udelukkelsesmetoden kan man bare sætte CE low, så at man er pr. automatik i Standby-I mode og ikke laver packet transmission
	
	NRF24CSNHigh(); // Når CSN = LOW --> Chip er valgt (aktiv), Når CSN = HIGH --> Chip er ikke valgt (inaktiv) på SPI
	// Observer timing diagram også at når CSN er høj, så er nRF24L01 IKKE aktiv på SPI (page 47 på datablad af nRF24)
}

static void NRF24WriteCommand(uint8_t cmd) // Kig på timing diagrammet på nRF24L01
{
	NRF24CSNLow();
	NRF24SPITransceive(cmd);
	NRF24CSNHigh();
}

static void NRF24StartUpDelay(void) // Transition State (Observer legend i state machine diagram for radio control)
{
	// Tpd2stby - PowerDown --> Standby mode : MAX 1.5 ms MED INTERNAL CRYSTAL OSCILLATOR
	// Siden MAX --> "Efter 1.5 ms er vi GARANTERET klar" --> Put derfor en buffer på 2 ms
	_delay_ms(2);
	
	// KIG PÅ STATE MACHINE DIAGRAMMET FOR RADIO CONTROL
	// Altså, når nRF24 har power --> Så sættes "PWR_UP" til 1
	// DER GÅR I HVERTFALD 1.5 ms (kaldt "Start Up") --> DERMED ER MAN GARANTERET AT VÆRE I "Standby-I" mode
}

static void NRF24TX_RXSettlingDelay(void) // Transition State (Observer legend i state machine diagram for radio control)
{
	// Tstby2a - Standby modes --> TX/RX mode : MAX 130 us
	// Siden MAX --> "Efter 130 us er vi GARANTERET klar" --> Put derfor en buffer på 200 us	
	_delay_us(200);
	
	// KIG PÅ STATE MACHINE DIAGRAMMET FOR RADIO CONTROL
	// Altså når nRF24 er i state "Standby-I", så sættes "CE" til 1
	// DER GÅR I HVERTFALD 130 us (kaldt "RX Settling" ELLER "TX Settling") --> DERMED ER MAN GARANTERET AT VÆRE I ENTEN RX MODE ELLER TX MODE, AFHÆNGIGT AF NRF_CONFIG_PRIM_RX-bit'en
}

static void NRF24PowerOnResetDelay(void) // Transition State (Observer legend i state machine diagram for radio control)
{
	// Datasheet: TPOR max 10.3 ms after VDD reaches 1.9 V
	_delay_ms(11);
}

// PUBLIC FUNCTIONS ////////////////////////////////////////////////////////////

void NRF24Init(void)
{
    NRF24PinsInit();
    NRF24PowerOnResetDelay();
    NRF24SPIInit();
}

uint8_t NRF24GetStatus(void)
{
	uint8_t status;

	NRF24CSNLow();
	status = NRF24SPITransceive(NRF_CMD_NOP);
	NRF24CSNHigh();

	return status;
}

uint8_t NRF24ReadRegister(uint8_t reg)
{
	uint8_t data;

	NRF24CSNLow();
	NRF24SPITransceive(NRF_CMD_R_REGISTER | (NRF_CMD_REGISTER_MAP_ADDRESS & reg));
	data = NRF24SPITransceive(NRF_CMD_NOP);
	NRF24CSNHigh();

	return data;
}

void NRF24WriteRegister(uint8_t reg, uint8_t value)
{
	NRF24CSNLow();
	NRF24SPITransceive(NRF_CMD_W_REGISTER | (NRF_CMD_REGISTER_MAP_ADDRESS & reg));
	NRF24SPITransceive(value);
	NRF24CSNHigh();
}

void NRF24ReadRegisterMulti(uint8_t reg, uint8_t *data, uint8_t length)
{
	NRF24CSNLow();
	NRF24SPITransceive(NRF_CMD_R_REGISTER | (NRF_CMD_REGISTER_MAP_ADDRESS & reg));

	for (uint8_t i = 0; i < length; i++) {
		data[i] = NRF24SPITransceive(NRF_CMD_NOP);
	}

	NRF24CSNHigh();
}

void NRF24WriteRegisterMulti(uint8_t reg, const uint8_t *data, uint8_t length)
{
	NRF24CSNLow();
	NRF24SPITransceive(NRF_CMD_W_REGISTER | (NRF_CMD_REGISTER_MAP_ADDRESS & reg));

	for (uint8_t i = 0; i < length; i++) {
		NRF24SPITransceive(data[i]);
	}

	NRF24CSNHigh();
}

void NRF24WritePayload(const uint8_t *data, uint8_t length)
{
	NRF24CSNLow();
	NRF24SPITransceive(NRF_CMD_W_TX_PAYLOAD);

	for (uint8_t i = 0; i < length; i++) {
		NRF24SPITransceive(data[i]);
	}

	NRF24CSNHigh();
}

void NRF24ReadPayload(uint8_t *data, uint8_t length)
{
	NRF24CSNLow();
	NRF24SPITransceive(NRF_CMD_R_RX_PAYLOAD);

	for (uint8_t i = 0; i < length; i++) {
		data[i] = NRF24SPITransceive(NRF_CMD_NOP);
	}

	NRF24CSNHigh();
}

void NRF24FlushTX(void)
{
	NRF24WriteCommand(NRF_CMD_FLUSH_TX);
}

void NRF24FlushRX(void)
{
	NRF24WriteCommand(NRF_CMD_FLUSH_RX);
}

void NRF24ClearAllInterrupts(void)
{
	// Clear RX_DR, TX_DS, MAX_RT by writing 1s
	NRF24WriteRegister(NRF_REG_STATUS,
	(1 << NRF_STATUS_RX_DR) |
	(1 << NRF_STATUS_TX_DS) |
	(1 << NRF_STATUS_MAX_RT));
}

void NRF24InitTX(uint8_t channel, uint8_t payloadLength, const uint8_t *address)
{
	NRF24CELow();

	// Disable auto-ack
	NRF24WriteRegister(NRF_REG_EN_AA, 0x00);

	// Enable only pipe 0
	NRF24WriteRegister(NRF_REG_EN_RXADDR, 0x01);

	// 5-byte address width
	NRF24WriteRegister(NRF_REG_SETUP_AW, 0x03);

	// Disable retransmit
	NRF24WriteRegister(NRF_REG_SETUP_RETR, 0x00);

	// RF channel
	NRF24WriteRegister(NRF_REG_RF_CH, channel & 0x7F);

	// RF setup:
	// bit 0 = LNA_HCURR
	// RF_DR = 0 => 1 Mbps
	// RF_PWR = 11 => 0 dBm
	NRF24WriteRegister(NRF_REG_RF_SETUP, 0x07);

	// Set TX address
	NRF24WriteRegisterMulti(NRF_REG_TX_ADDR, address, 5);

	// Optional: set RX_ADDR_P0 equal to TX_ADDR
	// useful later if auto-ack is enabled
	NRF24WriteRegisterMulti(NRF_REG_RX_ADDR_P0, address, 5);

	// Static payload length for pipe 0
	NRF24WriteRegister(NRF_REG_RX_PW_P0, payloadLength & 0x3F);

	NRF24FlushTX();
	NRF24FlushRX();
	NRF24ClearAllInterrupts();

	// CONFIG:
	// EN_CRC = 1
	// PWR_UP = 1
	// PRIM_RX = 0 (TX mode)
	NRF24WriteRegister(NRF_REG_CONFIG,
	(1 << NRF_CONFIG_EN_CRC) |
	(1 << NRF_CONFIG_PWR_UP));

	NRF24StartUpDelay();
}

void NRF24InitRX(uint8_t channel, uint8_t payloadLength, const uint8_t *address)
{
	NRF24CELow();

	// Disable auto-ack
	NRF24WriteRegister(NRF_REG_EN_AA, 0x00);

	// Enable only pipe 0
	NRF24WriteRegister(NRF_REG_EN_RXADDR, 0x01);

	// 5-byte address width
	NRF24WriteRegister(NRF_REG_SETUP_AW, 0x03);

	// Disable retransmit
	NRF24WriteRegister(NRF_REG_SETUP_RETR, 0x00);

	// RF channel
	NRF24WriteRegister(NRF_REG_RF_CH, channel & 0x7F);

	// RF setup: 1 Mbps, 0 dBm
	NRF24WriteRegister(NRF_REG_RF_SETUP, 0x07);

	// Set RX address pipe 0
	NRF24WriteRegisterMulti(NRF_REG_RX_ADDR_P0, address, 5);

	// Static payload length for pipe 0
	NRF24WriteRegister(NRF_REG_RX_PW_P0, payloadLength & 0x3F);

	NRF24FlushTX();
	NRF24FlushRX();
	NRF24ClearAllInterrupts();

	// CONFIG:
	// EN_CRC = 1
	// PWR_UP = 1
	// PRIM_RX = 1 (RX mode)
	NRF24WriteRegister(NRF_REG_CONFIG,
	(1 << NRF_CONFIG_EN_CRC) |
	(1 << NRF_CONFIG_PWR_UP) |
	(1 << NRF_CONFIG_PRIM_RX));

	NRF24StartUpDelay();
	
	NRF24CEHigh(); // Enable CE først, bagefter lav RX/TX settling delay (ifølge state machine diagrammet)
	
	NRF24TX_RXSettlingDelay();
}

bool NRF24SendPacket(const uint8_t *data, uint8_t length)
{
    NRF24CELow();
    NRF24ClearAllInterrupts();
    NRF24FlushTX();
    NRF24WritePayload(data, length);

    NRF24CEHigh();
    _delay_us(15);
    NRF24CELow();

    for (uint16_t i = 0; i < 1000; i++) {
        uint8_t status = NRF24GetStatus();

        if (status & (1 << NRF_STATUS_TX_DS)) {
            NRF24ClearAllInterrupts();
            return true;
        }

        if (status & (1 << NRF_STATUS_MAX_RT)) {
            NRF24ClearAllInterrupts();
            NRF24FlushTX();
            return false;
        }

        _delay_us(20);
    }

    return false;
}

bool NRF24DataReady(void)
{
	uint8_t fifo = NRF24ReadRegister(NRF_REG_FIFO_STATUS);

	// RX_EMPTY = 0 means data available
	return ((fifo & (1 << NRF_FIFO_RX_EMPTY)) == 0);
}

void NRF24ReceivePacket(uint8_t *data, uint8_t length)
{
	NRF24ReadPayload(data, length);
	NRF24ClearAllInterrupts();
}
