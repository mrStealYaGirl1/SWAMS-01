/************************************************************
File name: "Touchdriver.c"

Driver for touch screen access, perhaps also pressure differentiation - XPT2046 module, Version 2007.5
Mounted at "ITDB02 Arduino Mega2560 Shield".
Display controller = XPT2046.

Max. uC clock frequency = 16 MHz (Tclk = 62,5 ns)

Connections:
CLK:		PORT H, bit 3
CS:			PORT E, bit 3
DIN:		PORT G, bit 5
DOUT:		PORT E, bit 5
IRQ:		PORT E, bit 4 (= INT 4)

Christian Rex Rønfeldt Brandt Pilegaard
************************************************************/
#pragma once
#include <stdint.h>
#include <stdbool.h>


typedef struct {
	uint16_t x; // 0..319
	uint16_t y; // 0..249
} TouchPoint;

void TouchInit(void);

// Return true if "pen touched" - Activate Pen Interrupt
// True når PENIRQ = low (trykket)
bool TouchPressed(void);

bool TouchReadRaw(uint16_t *x, uint16_t *y);

bool TouchGetPoint(TouchPoint *p);