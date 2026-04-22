/**
* @file SD_Driver.c
* @brief Implementation of SD/SDHC communication routines.
* @author Aarhus Universitet, Michael Alr?e
*/

#include <avr/io.h>
#include "SD_Driver.h"
#include "SPI_Driver.h"

#define ON     1
#define OFF    0

static uint8_t SDHC_flag = 0; // Flag for High Capacity support
static uint8_t cardType = 0;  // Stores the detected card type
static uint8_t CRC_Mode = 0;  // Tracks if CRC is currently enabled

uint8_t SD_getCardType(void) { return cardType; }
uint8_t SD_getSDHCflag(void) { return SDHC_flag; }

uint8_t SD_init(void) {
	uint8_t i;
	uint16_t retry = 0;
	uint8_t response;
	
	CRC_Mode = ON;
	SPI_init();
	
	// Provide 80 clock pulses while CS is high to wake the card
	SPI_Chip_Deselect();
	for(i = 0; i < 10; i++) {
		SPI_transmit(0xFF);
	}

	SPI_Chip_Select();
	
	// CMD0: Put card in SPI idle state
	retry = 0;
	do {
		response = SD_sendCommand(CMD0_GO_IDLE_STATE, 0, 0x95);
		if(retry++ > 0x200) return 1;
	} while(response != 0x01);

	// CMD8: Check voltage and card version (Mandatory for SDHC support)
	response = SD_sendCommand(CMD8_SEND_IF_COND, 0x000001AA, 0x87);
	if(response == 0x01) {
		for(i = 0; i < 4; i++) SPI_transmit(0xFF); // Read 4-byte R7 response
		cardType = 2; // Potential SDHC (Ver 2.0+)
		} else {
		cardType = 1; // SDSC (Ver 1.x)
	}

	// ACMD41: Card initialization
	retry = 0;
	do {
		SD_sendCommand(CMD55_APP_CMD, 0, 0x65);
		// Send HCS bit (High Capacity Support) in argument
		response = SD_sendCommand(ACMD41_SD_SEND_OP_COND, 0x40000000, 0x77);
		if(retry++ > 0xFFE) return 2;
	} while(response != 0x00); // Wait until initialization is complete

	// CMD58: Read OCR register to check the CCS (Card Capacity Status) bit
	response = SD_sendCommand(CMD58_READ_OCR, 0, 0xFF);
	if(response == 0x00) {
		uint8_t ocr_byte1 = SPI_transmit(0xFF); // First byte of OCR contains CCS bit
		for(i = 0; i < 3; i++) SPI_transmit(0xFF); // Remaining 3 bytes
		
		// CCS bit (bit 30) indicates block addressing (SDHC/SDXC)
		if (ocr_byte1 & 0x40) {
			SDHC_flag = 1;
			cardType = 2;
			} else {
			SDHC_flag = 0;
			cardType = 3;
		}
	}

	// Disable CRC for subsequent communication
	SD_sendCommand(CMD59_CRC_ON_OFF, OFF, 0xFF);
	CRC_Mode = OFF;
	
	SPI_Chip_Deselect();
	return 0;
}

uint8_t SD_sendCommand(SD_Command_t cmd, uint32_t arg, uint8_t crc) {
	uint8_t response, retry = 0;
	
	// Address conversion: SDSC uses byte addresses, SDHC uses block addresses!
	if(SDHC_flag == 0) {
		if(cmd == CMD17_READ_SINGLE_BLOCK || cmd == CMD24_WRITE_SINGLE_BLOCK ||
		cmd == CMD32_ERASE_BLOCK_START_ADDR || cmd == CMD33_ERASE_BLOCK_END_ADDR) {
			arg <<= 9;
		}
	}

	SPI_transmit((uint8_t)cmd | 0x40);
	SPI_transmit(arg >> 24);
	SPI_transmit(arg >> 16);
	SPI_transmit(arg >> 8);
	SPI_transmit(arg);
	SPI_transmit(CRC_Mode ? crc : 0xFF);
	
	while((response = SPI_receive()) == 0xFF) {
		if(retry++ > 0xFE) break;
	}
	return response;
}

uint8_t SD_readSingleBlock(uint32_t startBlock, uint8_t* ptr) {
	uint8_t res;
	uint16_t i, retry = 0;

	SPI_Chip_Select();
	res = SD_sendCommand(CMD17_READ_SINGLE_BLOCK, startBlock, 0x00);
	
	if (res != 0x00) { // Fejl i kommando
		SPI_Chip_Deselect();
		return res;
	}

	// Vent på Data Token (0xFE)
	while (SPI_receive() != 0xFE) {
		if (retry++ > 0xFFFE) {
			SPI_Chip_Deselect();
			return 0xFF; // Timeout
		}
	}

	// Læs 512 bytes data
	for (i = 0; i < 512; i++) {
		ptr[i] = SPI_receive();
	}

	// Læs og smid 2 bytes CRC væk
	SPI_receive();
	SPI_receive();

	SPI_Chip_Deselect();
	return 0x00; // Success
}

uint8_t SD_writeSingleBlock(uint32_t startBlock, const uint8_t* ptr) {
	uint8_t res;
	uint16_t i, retry = 0;

	SPI_Chip_Select();
	res = SD_sendCommand(CMD24_WRITE_SINGLE_BLOCK, startBlock, 0x00);
	if (res != 0x00) {
		SPI_Chip_Deselect();
		return res;
	}

	// Send Data Token før data (0xFE for single block)
	SPI_transmit(0xFE);

	// Send 512 bytes data
	for (i = 0; i < 512; i++) {
		SPI_transmit(ptr[i]);
	}

	// Send 2 bytes dummy CRC
	SPI_transmit(0xFF);
	SPI_transmit(0xFF);

	// Tjek Data Response (skal være xxxx0101 for accept, typisk 0x05)
	res = SPI_receive();
	if ((res & 0x1F) != 0x05) { // Fejl
		SPI_Chip_Deselect();
		return 0xFF;
	}

	while (SPI_receive() == 0x00) {
		if (retry++ > 0xFFFE) break;
	}

	SPI_Chip_Deselect();
	return 0x00; // Success
}

uint8_t SD_erase(uint32_t startBlock, uint32_t totalBlocks) {
	uint8_t res = 0;
	SPI_Chip_Select();
	SD_sendCommand(CMD32_ERASE_BLOCK_START_ADDR, startBlock, 0x00);
	SD_sendCommand(CMD33_ERASE_BLOCK_END_ADDR, startBlock+totalBlocks - 1, 0x00);
	res = SD_sendCommand(CMD38_ERASE_SELECTED_BLOCKS, startBlock, 0x00);
	
	uint16_t timeout = 0xFFFF;
	while (SPI_receive() == 0x00 && timeout > 0) {
		timeout--;
	}
	
	SPI_Chip_Deselect();
	return res;
}
