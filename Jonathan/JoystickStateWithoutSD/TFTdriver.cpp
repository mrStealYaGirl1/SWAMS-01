/************************************************************
File name: "TFTdriver.c"

Driver for "ITDB02 320 x 240 TFT display module, Version 2"
mounted at "ITDB02 Arduino Mega2560 Shield".
Display controller = ILI 9341.

Max. uC clock frequency = 16 MHz (Tclk = 62,5 ns)

Connections:
DB15-DB8:   PORT A
DB7-DB0:    PORT C

RESETx:     PORT G, bit 0
CSx:        PORT G, bit 1
WRx:        PORT G, bit 2
RS (=D/Cx): PORT D, bit 7

Henning Hargaard
Modified Michael Alrøe
************************************************************/
#include <avr/io.h>

#define F_CPU 16000000
#include <util/delay.h>

#include <avr/cpufunc.h>  // _NOP()
#include "TFTdriver.h"

// Data port definitions:
#define DATA_PORT_HIGH PORTA
#define DATA_PORT_LOW  PORTC

// Control port definitions:
#define WR_PORT PORTG
#define WR_BIT 2
#define DC_PORT PORTD
#define DC_BIT  7  //"DC" signal is at the shield called RS
#define CS_PORT PORTG
#define CS_BIT  1
#define RST_PORT PORTG
#define RST_BIT 0

// LOCAL FUNCTIONS /////////////////////////////////////////////////////////////

// ILI 9341 data sheet, page 238
static void TFTWriteCommand(uint8_t command) // BEWARE; COMMANDS ONLY USE THE LOWER 8 BITS, THAT IS ALL OF PORTC "DATA_PORT_LOW"
{
	DATA_PORT_HIGH = 0x00;     // <-- vigtigt på 16-bit bus
	// Data set-up (commands only use the lower byte of the data bus)
	DATA_PORT_LOW = command;
	
	// DC low
	DC_PORT &= ~(1 << DC_BIT); // Behold alt – men sørg for at denne bit (WR_BIT) er 0

	// Deactivate SD
	PORTB |= (1 << PB0);

	// CS low
	CS_PORT &= ~(1 << CS_BIT); // Behold alt – men sørg for at denne bit (WR_BIT) er 0
	
	// WR low
	WR_PORT &= ~(1 << WR_BIT); // Behold alt – men sørg for at denne bit (WR_BIT) er 0
	
	// twr0 > 15 ns
	_NOP(); // 1/16MHz = 62.5 nS - about 4.2 times slower

	// WR high
	WR_PORT |= (1 << WR_BIT); // Behold alt - men sørg for at denne bit (WR_BIT) er 1

}

// ILI 9341 data sheet, page 238
static void TFTWriteData(uint16_t data) // BEWARE; DATA USES ALL 16 BITS, COMPARED TO COMMANDS, SO BOTH "DATA"PORT_HIGH" (MSByte) AND "DATA_PORT_LOW" (LSByte)
{
	// Data set up
	DATA_PORT_HIGH = (uint8_t)(data >> 8); // Skubber bitsne op til de 8 mest signifikante bits
	DATA_PORT_LOW = (uint8_t)(data); // De resterende 8-bits sættes til lower port
	
	// DC high
	DC_PORT |= (1 << DC_BIT);
	
	// Deactivate SD
	PORTB |= (1 << PB0);
	
	// CS low
	CS_PORT &= ~(1 << CS_BIT);

	// WR low
	WR_PORT &= ~(1 << WR_BIT); 

	// twr0 > 15 ns
	_NOP();
	
	// WR high
	WR_PORT |= (1 << WR_BIT); 

}

// PUBLIC FUNCTIONS ////////////////////////////////////////////////////////////

// Initializes (resets) the display
void TFTDisplayInit() // Nessescary initialization
{
	// Control pins are outputs
	
		// WR og CS er port G - ALSO RESET?
	DDRG |= ( (1 << CS_BIT) | (1 << WR_BIT) | (1 << RST_BIT) );
	
		// DC er port D 
	DDRD |= (1 << DC_BIT);
	
	// Data pins are outputs
	
		// "DATA_PORT_HIGH" (D[15:8]) styres af PORTA --> DDRA
	DDRA |= 0b11111111;
		// "DATA_PORT_LOW" (D[7:0]) styres af PORTC --> DDRC
	DDRC |= 0b11111111;
	
	// All control pins high;
	CS_PORT |= (1 << CS_BIT);
	WR_PORT |= (1 << WR_BIT);
	DC_PORT |= (1 << DC_BIT);
	
	// RST low
	RST_PORT &= ~(1 << RST_BIT);

	// Wait 500 ms
	_delay_ms(500);
	
	// RST high + Wait
	RST_PORT |= (1 << RST_BIT);
	_delay_ms(500);
	
	// Exit sleep mode
	TFTSleepOut();
	
	// Display on
	TFTDisplayOn();

	// Set bit BGR (scanning direction)	
	//TFTMemoryAccessControl(0b00001000); // OBS: Argument skal være "parameter" ikke "command", som ses i samme datablad
	// Parameteren er at vi skal sætte BGR bitten høj, for at kompensere for et BGR-panel.
	
	TFTMemoryAccessControl(0x28); // MV + BGR  (landscape)
	
	// 16 bits (2 bytes) per pixel
	TFTInterfacePixelFormat(0b01010101); // Selects R-G-B in 5-6-5 mode, cause of 16-bit . OBS: Argument skal være "parameter" ikke "command", som ses i samme datablad
		// DPI[2:0] = 101 --> 16 bits / pixel
		// DBI[2:0] = 101 --> 16 bits / pixel
}

void TFTDisplayOff() // Turn off the display
{
	// Write "DISPOFF" (Display OFF) command
	TFTWriteCommand(0b00101000);
}

void TFTDisplayOn() // Turn on the display
{	
	// Write "DISPON" (Display ON) command
	TFTWriteCommand(0b00101001);
}

void TFTSleepOut() // Bring the display OUT of sleep mode
{
	// Write "SLPOUT" (Sleep Out) command
	TFTWriteCommand(0b00010001);
	
	//It takes 120msec to become Sleep Out mode after SLPOUT command issued.
	_delay_ms(150); // 30 msec buffer
}

void TFTMemoryAccessControl(uint8_t parameter) // Set the right "Access" mode for the module
{
	// Write "MADCTL" (Memory Access Control) command
	TFTWriteCommand(0b00110110);
	
	// LÆG MÆRKE TIL AT DC (SOM BETYDER "D" (DATA) OG "C" (COMMAND)), ALTSÅ DC ER 1, NÅR DEN TAGER IMOD PARAMETER. DVS. AT VI SKAL SENDE DATA
	
	TFTWriteData(parameter);
}

void TFTInterfacePixelFormat(uint8_t parameter) // Will select R-G-B = 5-6-5 (16 bits)
{
	// Write "PIXSET" (Pixel Format Set) command
	TFTWriteCommand(0b00111010);
	
	// LÆG MÆRKE TIL AT DC (SOM BETYDER "D" (DATA) OG "C" (COMMAND)), ALTSÅ DC ER 1, NÅR DEN TAGER IMOD PARAMETER. DVS. AT VI SKAL SENDE DATA
	TFTWriteData(parameter);
}

void TFTMemoryWrite() // Send the Memory Write command
{
	// Write "RAMWR" (Memory Write) command
	TFTWriteCommand(0b00101100);
}

// Red 0-31, Green 0-63, Blue 0-31
void TFTWritePixel(uint8_t Red, uint8_t Green, uint8_t Blue) // Send one colour pixel data to the display RAM
{
	// R-G-B = 5-6-5
	
	// Red - D[15:11] - 5 --> DVS. DER ER 5 bits opløsning i rød farve, som er 31 forskellige værdier

	// Green - D[10:5] - 6 --> 63 forskellige grønne farver
	
	// Blue - D[4:0] - 5 --> 31 forskellige blå farver
	
	// ALTSÅ VI SKAL OPSTILLE DE 16 bit data, SOM "Red", "Green", "Blue" tilhører. Dernæst skal vi så sende det data.
	
	uint16_t pixel = ((uint16_t)(Red & 0b00011111) << 11) | ((uint16_t)(Green & 0b00111111) << 5) | ((uint16_t)(Blue & 0b00011111));
	// Tildel rød til de 5 højeste datapins. Ved at shifte 11 så går vi fra: 00000 000000 11111 til 11111 000000 00000
	
	// Tildel grøn til de 6 miderste datapints. Ved at shite 5 gange, så går vi fra: 00000 000001 11111 til 00000 111111 00000
	
	// Tildel de sidste til blå. Her behøves ingen shifting, da man allerede i binary form skriver hvor de tilhøjrer, altså: 00000 000000 11111
	
	// Nu kan vi sende data:
	TFTWriteData(pixel);
	
}

// Set Column Address (0-239), Start > End
void TFTSetColumnAddress(uint16_t Start, uint16_t End) // Set the specified column address
{
	// Clamping the column addressing
	/*if (Start > 239)
	Start = 239;
	*/
	
	if (Start > 319) Start = 319;

	// HUSK AT UNSIGNED DATATYPER ALDRIG KAN VÆRE NEGATIVE! DET ER ALTID ABSOLUTTE VÆRDIER! :DD

	/*if (End > 239)
	End = 239;
	*/
	
	if (End   > 319) End   = 319;
	
	// Write "CASET" (Column Address Set) command
	TFTWriteCommand(0b00101010);
	
	// 1st parameter - Assign Start to MSByte (8 bit)
	TFTWriteData((uint8_t)(Start >> 8));
	
	// 2nd parameter - Assign Start to LSByte (8 bit)
	TFTWriteData((uint8_t)(Start));
	
	// 3rd parameter - Assign End to MSByte (8 bit)
	TFTWriteData((uint8_t)(End >> 8));
	
	// 4th parameter - Assign End to LSByte (8 bit)
	TFTWriteData((uint8_t)(End));
}

// Set Page Address (0-319), Start > End
void TFTSetPageAddress(uint16_t Start, uint16_t End) // Set the specifdied page address
{
	// Clamping the page addressing
	/*if (Start > 319)
	Start = 319;

	if (End > 319)
	End = 319;*/
	
	
	// Write "PASET" (Page Address Set) command
	TFTWriteCommand(0b00101011);

	// 1st parameter - Assign Start to MSByte (8 bit)
	TFTWriteData((uint8_t)(Start >> 8));
	
	// 2nd parameter - Assign Start to LSByte (8 bit)
	TFTWriteData((uint8_t)(Start));
	
	// 3rd parameter - Assign End to MSByte (8 bit)
	TFTWriteData((uint8_t)(End >> 8));
	
	// 4th parameter - Assign End to LSByte (8 bit)
	TFTWriteData((uint8_t)(End));
}

// Fills rectangle with specified color
// (StartX,StartY) = Upper left corner. X horizontal (0-319) , Y vertical (0-239).
// Height (1-240) is vertical. Width (1-320) is horizontal.
// R-G-B = 5-6-5 bits.
void TFTFillRectangle(uint16_t StartX, uint16_t StartY, uint16_t Width,
uint16_t Height, uint8_t Red, uint8_t Green, uint8_t Blue)
{
	if (StartX > 319) StartX = 319;
	if (StartY > 239) StartY = 239;

	// beregn End uden overflow
	uint16_t endX = StartX;
	uint16_t endY = StartY;

	if (Width > 0) {
		if (Width - 1 > (uint16_t)(319 - StartX)) endX = 319;
		else endX = StartX + (Width - 1);
	}

	if (Height > 0) {
		if (Height - 1 > (uint16_t)(239 - StartY)) endY = 239;
		else endY = StartY + (Height - 1);
	}

	//TFTSetColumnAddress(StartY, endY);
	//TFTSetPageAddress(StartX, endX);
	
	TFTSetColumnAddress(StartX,endX);
	TFTSetPageAddress(StartY, endY);
	
	TFTMemoryWrite();

	for (uint16_t a = 0; a < (endY - StartY + 1); a++) {
		for (uint16_t b = 0; b < (endX - StartX + 1); b++) {
			TFTWritePixel(Red, Green, Blue);
		}
	}
}