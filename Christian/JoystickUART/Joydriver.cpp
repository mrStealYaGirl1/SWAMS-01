#include "Joydriver.h"

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

// ---------- Pin mapping ----------
#define JOY_ADC_CH_X   0   // ADC0 / A0 (PF0)
#define JOY_ADC_CH_Y   1   // ADC1 / A1 (PF1)

// Button on PL6 (active LOW)
#define JOY_SW_DDR     DDRL
#define JOY_SW_PORT    PORTL
#define JOY_SW_PINR    PINL
#define JOY_SW_BIT     6

// ---------- Default tuning ----------
#define JOY_CENTER_X   512
#define JOY_CENTER_Y   512
#define JOY_DEADZONE   90
#define JOY_THRESHOLD  240

static inline bool joy_sw_pressed()
{
	// active LOW
	return (JOY_SW_PINR & (1 << JOY_SW_BIT)) == 0;
}

// ADC read for channels 0..7 (ADC0..ADC7) -> no MUX5 needed
static inline uint16_t adc_read_0to7(uint8_t ch)
{
	ADMUX = (1 << REFS0) | (ch & 0x07);  // AVcc ref + channel
	ADCSRA |= (1 << ADSC);              // start conversion
	while (ADCSRA & (1 << ADSC)) {}     // wait
	return ADC;                         // 10-bit result
}

void JoyInit(void)
{
	// AVcc reference, ADC enable, prescaler 128 (16MHz -> 125kHz ADC clock)
	ADMUX  = (1 << REFS0);
	ADCSRB = 0;
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

	// Button input + pull-up
	JOY_SW_DDR  &= ~(1 << JOY_SW_BIT);
	JOY_SW_PORT |=  (1 << JOY_SW_BIT);

	// Dummy conversion to stabilize first read
	(void)adc_read_0to7(JOY_ADC_CH_X);
}

bool JoyReadRaw(JoyRaw *out)
{
	if (!out) return false;

	out->x  = adc_read_0to7(JOY_ADC_CH_X);
	out->y  = adc_read_0to7(JOY_ADC_CH_Y);
	out->sw = joy_sw_pressed();

	return true;
}

JoyDir JoyGetDirDefault(uint16_t x, uint16_t y)
{
	return JoyGetDir(x, y, JOY_CENTER_X, JOY_CENTER_Y, JOY_DEADZONE, JOY_THRESHOLD);
}

JoyDir JoyGetDir(uint16_t x, uint16_t y,
uint16_t centerX,
uint16_t centerY,
uint16_t deadzone,
uint16_t threshold)
{
	int16_t dx = (int16_t)x - (int16_t)centerX;
	int16_t dy = (int16_t)y - (int16_t)centerY;

	// deadzone around center
	if ((dx > -(int16_t)deadzone && dx < (int16_t)deadzone) &&
	(dy > -(int16_t)deadzone && dy < (int16_t)deadzone))
	{
		return JOY_NONE;
	}

	int16_t adx = (dx < 0) ? -dx : dx;
	int16_t ady = (dy < 0) ? -dy : dy;

	// choose dominant axis for stability
	if (adx >= ady) {
		if (adx < (int16_t)threshold) return JOY_NONE;
		return (dx > 0) ? JOY_RIGHT : JOY_LEFT;
		} else {
		if (ady < (int16_t)threshold) return JOY_NONE;

		// If your joystick has inverted Y, swap these:
		// return (dy > 0) ? JOY_UP : JOY_DOWN;
		return (dy > 0) ? JOY_DOWN : JOY_UP;
	}
}