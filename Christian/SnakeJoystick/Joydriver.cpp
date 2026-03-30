#include "Joydriver.h"
#include <avr/io.h>

// =======================
// Pin-mapping (RET: ret disse)
// =======================

// VRx = ADC channel
#define JOY_ADC_CH_X   0   // A0
#define JOY_ADC_CH_Y   1   // A1

// SW button pin (aktiv LOW typisk)
#define JOY_SW_DDR     DDRL
#define JOY_SW_PORT    PORTL
#define JOY_SW_PINR    PINL
#define JOY_SW_BIT     6   // PL6 (vælg den du faktisk forbinder til)

// =======================
// Default “kalibrering” (kan justeres)
// =======================
static const uint16_t JOY_CENTER_X = 512;
static const uint16_t JOY_CENTER_Y = 512;
static const uint16_t JOY_DEADZONE = 90;    // område omkring center der ignoreres
static const uint16_t JOY_THRESHOLD = 240;  // hvor langt ud før retning gælder

// =======================
// ADC low-level
// =======================

// Læs ADC channel 0..15 på ATmega2560
static uint16_t adc_read(uint8_t ch)
{
	// Channels 0..7: MUX5=0, MUX[3:0]=ch
	// Channels 8..15: MUX5=1, MUX[3:0]=ch-8
	if (ch > 15) ch = 15;

	// Set MUX5 in ADCSRB (bit 3) for channels 8..15
	if (ch & 0x08) {
		ADCSRB |=  (1 << MUX5);
		ch &= 0x07;
		} else {
		ADCSRB &= ~(1 << MUX5);
	}

	// ADMUX: REFS0=1 => AVcc ref, right-adjusted, select channel
	ADMUX = (1 << REFS0) | (ch & 0x0F);

	// start conversion
	ADCSRA |= (1 << ADSC);

	// wait
	while (ADCSRA & (1 << ADSC)) {}

	// read 10-bit
	return ADC; // reads ADCL+ADCH correctly
}

void JoyInit(void)
{
	// ADC enable, prescaler=128 (16MHz/128 = 125kHz)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // :contentReference[oaicite:2]{index=2}
	ADCSRB = 0x00;
	ADMUX  = (1 << REFS0); // AVcc reference

	// “dummy” conversion (stabiliserer første read)
	(void)adc_read(JOY_ADC_CH_X);

	// Button input + pull-up
	JOY_SW_DDR  &= ~(1 << JOY_SW_BIT);
	JOY_SW_PORT |=  (1 << JOY_SW_BIT);
}

static inline bool joy_sw_pressed(void)
{
	// aktiv LOW
	return ((JOY_SW_PINR & (1 << JOY_SW_BIT)) == 0);
}

bool JoyReadRaw(JoyRaw *out)
{
	if (!out) return false;

	// Læsning: hvis du vil glatte mere, kan du median/average her.
	uint16_t x = adc_read(JOY_ADC_CH_X);
	uint16_t y = adc_read(JOY_ADC_CH_Y);

	out->x  = x;
	out->y  = y;
	out->sw = joy_sw_pressed();
	return true;
}

JoyDir JoyGetDir(uint16_t x, uint16_t y,
uint16_t centerX, uint16_t centerY,
uint16_t deadzone, uint16_t threshold)
{
	int16_t dx = (int16_t)x - (int16_t)centerX;
	int16_t dy = (int16_t)y - (int16_t)centerY;

	// deadzone
	if (dx < (int16_t)deadzone && dx > -(int16_t)deadzone &&
	dy < (int16_t)deadzone && dy > -(int16_t)deadzone)
	{
		return JOY_NONE;
	}

	// vælg dominerende akse (mere stabilt end “4 thresholds”)
	int16_t adx = (dx < 0) ? -dx : dx;
	int16_t ady = (dy < 0) ? -dy : dy;

	if (adx >= ady) {
		if (adx < (int16_t)threshold) return JOY_NONE;
		return (dx > 0) ? JOY_RIGHT : JOY_LEFT;
		} else {
		if (ady < (int16_t)threshold) return JOY_NONE;
		// OBS: nogle joysticks har “op” = lavere spænding på Y. Byt evt. rundt her:
		return (dy > 0) ? JOY_DOWN : JOY_UP;
	}
}

JoyDir JoyGetDirDefault(uint16_t x, uint16_t y)
{
	return JoyGetDir(x, y, JOY_CENTER_X, JOY_CENTER_Y, JOY_DEADZONE, JOY_THRESHOLD);
}