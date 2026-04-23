#ifndef JOYDRIVER_H_
#define JOYDRIVER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint16_t x;   // 0..1023 (ADC)
	uint16_t y;   // 0..1023 (ADC)
	bool     sw;  // true = pressed
} JoyRaw;

typedef enum {
	JOY_NONE = 0,
	JOY_UP,
	JOY_DOWN,
	JOY_LEFT,
	JOY_RIGHT
} JoyDir;

// Init ADC + button input (pull-up)
void JoyInit(void);

// Læs rå joystick (ADC + button)
bool JoyReadRaw(JoyRaw *out);

// Returner retning baseret på deadzone + threshold.
// centerX/centerY er typisk ~512.
// deadzone: fx 60-120.
// threshold: fx 200-300 (hvor hårdt man skal “skubbe” før retning gælder).
JoyDir JoyGetDir(uint16_t x, uint16_t y,
uint16_t centerX, uint16_t centerY,
uint16_t deadzone, uint16_t threshold);

// Convenience: brug interne kalibreringskonstanter (nedenfor i .c)
JoyDir JoyGetDirDefault(uint16_t x, uint16_t y);

#endif