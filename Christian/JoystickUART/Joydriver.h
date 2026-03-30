#ifndef JOYDRIVER_H_
#define JOYDRIVER_H_

#include <stdint.h>
#include <stdbool.h>

// -------- Raw joystick data --------
typedef struct {
	uint16_t x;   // ADC value 0..1023
	uint16_t y;   // ADC value 0..1023
	bool     sw;  // true = button pressed
} JoyRaw;

// -------- Direction enum --------
typedef enum {
	JOY_NONE = 0,
	JOY_UP,
	JOY_DOWN,
	JOY_LEFT,
	JOY_RIGHT
} JoyDir;

// -------- Public functions --------

// Initialize ADC + button input
void JoyInit(void);

// Read raw joystick values
bool JoyReadRaw(JoyRaw *out);

// Convert raw values to direction using default thresholds
JoyDir JoyGetDirDefault(uint16_t x, uint16_t y);

// Optional generic version with custom thresholds
JoyDir JoyGetDir(uint16_t x, uint16_t y,
uint16_t centerX,
uint16_t centerY,
uint16_t deadzone,
uint16_t threshold);

#endif