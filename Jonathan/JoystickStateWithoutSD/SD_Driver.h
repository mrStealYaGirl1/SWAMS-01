/**
 * @file SD_Driver.h
 * @brief Driver for SD/SDHC/SDXC card communication via SPI.
 * @author Aarhus Universitet, Michael Alr?e
 */

#ifndef SD_DRIVER_H_
#define SD_DRIVER_H_

/**
 * @brief SD Card Commands for SPI mode
 */
typedef enum {
    CMD0_GO_IDLE_STATE          = 0,  // CMD0: Resets the SD card
    CMD1_SEND_OP_COND           = 1,  // CMD1: Activates the card's initialization process
    CMD8_SEND_IF_COND           = 8,  // CMD8: Sends SD Memory Card interface condition
    CMD9_SEND_CSD                = 9,  // CMD9: Sends Card Specific Data (CSD)
    CMD12_STOP_TRANSMISSION     = 12, // CMD12: Forces the card to stop transmission
    CMD13_SEND_STATUS           = 13, // CMD13: Asks the card to send its status register
    CMD16_SET_BLOCK_LEN         = 16, // CMD16: Sets a block length (in bytes)
    CMD17_READ_SINGLE_BLOCK     = 17, // CMD17: Reads a single block
    CMD18_READ_MULTIPLE_BLOCKS  = 18, // CMD18: Reads multiple blocks
    CMD24_WRITE_SINGLE_BLOCK    = 24, // CMD24: Writes a single block
    CMD25_WRITE_MULTIPLE_BLOCKS = 25, // CMD25: Writes multiple blocks
    CMD32_ERASE_BLOCK_START_ADDR = 32, // CMD32: Sets the address of the first block to be erased
    CMD33_ERASE_BLOCK_END_ADDR   = 33, // CMD33: Sets the address of the last block to be erased
    CMD38_ERASE_SELECTED_BLOCKS  = 38, // CMD38: Erases all previously selected blocks
    ACMD41_SD_SEND_OP_COND      = 41, // ACMD41: Application specific operating condition
    CMD55_APP_CMD               = 55, // CMD55: Indicates that the next command is an app specific command
    CMD58_READ_OCR              = 58, // CMD58: Reads the OCR register of a card
    CMD59_CRC_ON_OFF            = 59  // CMD59: Turns CRC option on or off
} SD_Command_t;

/**
 * @brief Initializes the SD/SDHC/SDXC card in SPI mode.
 * @details Performs power-on sequence, CMD0, CMD8, and ACMD41. 
 * Detects if the card uses block or byte addressing.
 * @return 0 on success, 1 on timeout (no card), 2 on initialization failure.
 */
uint8_t SD_init(void);

/**
 * @brief Sends a command to the SD card via SPI.
 * @param cmd The SD_Command_t enum value.
 * @param arg The 32-bit argument for the command.
 * @param crc The 7-bit CRC checksum.
 * @return The R1 response byte from the card.
 */
uint8_t SD_sendCommand(SD_Command_t cmd, uint32_t arg, uint8_t crc);

/**
 * @brief Reads a single 512-byte block from the card.
 * @param startBlock The block address to read.
 * @param ptr Pointer to a 512-byte buffer.
 * @return 0 on success, 1 on error/timeout.
 */
uint8_t SD_readSingleBlock(uint32_t startBlock, uint8_t* ptr);

/**
 * @brief Writes a single 512-byte block to the card.
 * @param startBlock The block address to write to.
 * @param ptr Pointer to the 512-byte data buffer.
 * @return 0 on success, or the error response byte from the card.
 */
uint8_t SD_writeSingleBlock(uint32_t startBlock, const uint8_t* ptr);

/**
 * @brief Erases a range of blocks on the card.
 * @param startBlock The first block to erase.
 * @param totalBlocks Number of blocks to include in the erase operation.
 * @return 0 on success, or error response byte.
 */
uint8_t SD_erase(uint32_t startBlock, uint32_t totalBlocks);

/**
 * @brief Gets the detected card type.
 * @return 1: SDSC (v1.x), 2: SDHC/SDXC, 3: SDSC (v2.0).
 */
uint8_t SD_getCardType(void);

/**
 * @brief Checks if the card is High Capacity (SDHC/SDXC).
 * @return 1 if the card uses block addressing, 0 if byte addressing.
 */
uint8_t SD_getSDHCflag(void);

#endif // SD_DRIVER_H_