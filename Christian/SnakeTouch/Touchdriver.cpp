#include "Touchdriver.h"
#include <avr/io.h>
#define F_CPU 16000000
#include <util/delay.h>
#include <avr/interrupt.h>

// Touch pins (fra opgaven)
#define T_CLK_DDR   DDRH
#define T_CLK_PORT  PORTH
#define T_CLK_PIN   3   // PH3

#define T_CS_DDR    DDRE
#define T_CS_PORT   PORTE
#define T_CS_PIN    3   // PE3

#define T_DIN_DDR   DDRG
#define T_DIN_PORT  PORTG
#define T_DIN_PIN   5   // PG5

#define T_DOUT_DDR  DDRE
#define T_DOUT_PINR PINE
#define T_DOUT_PIN  5   // PE5

#define T_IRQ_DDR   DDRE
#define T_IRQ_PINR  PINE
#define T_IRQ_PIN   4   // PE4 (INT4)

static inline void CLK0(void){ T_CLK_PORT &= ~(1<<T_CLK_PIN); }
static inline void CLK1(void){ T_CLK_PORT |=  (1<<T_CLK_PIN); }

static inline void CS0(void) { T_CS_PORT  &= ~(1<<T_CS_PIN); }
static inline void CS1(void) { T_CS_PORT  |=  (1<<T_CS_PIN); }

static inline void DIN0(void){ T_DIN_PORT &= ~(1<<T_DIN_PIN); }
static inline void DIN1(void){ T_DIN_PORT |=  (1<<T_DIN_PIN); }

static inline uint8_t DOUT(void){ return (T_DOUT_PINR & (1<<T_DOUT_PIN)) ? 1 : 0; }
static inline uint8_t IRQ(void) { return (T_IRQ_PINR  & (1<<T_IRQ_PIN))  ? 1 : 0; }

// Hold dig over timingkrav: tCH/tCL >= 200 ns
// 1 us er konservativt og virker fint til test.
static inline void tdelay(void){ _delay_us(1); }

static uint16_t xpt_transfer(uint8_t cmd)
{
	uint16_t r = 0;

	CS0();
	tdelay();

	// send 8 bits command (MSB first)
	for (uint8_t i=0; i<8; i++){
		if (cmd & 0x80) DIN1(); else DIN0();
		cmd <<= 1;

		// data latched on rising edge
		CLK0(); tdelay();
		CLK1(); tdelay();
	}

	// read 16 bits back
	// DOUT changes on falling edge -> stable around rising edge
	for (uint8_t i=0; i<16; i++){
		CLK0(); tdelay();
		CLK1(); tdelay();

		r <<= 1;
		r |= DOUT();
	}

	CS1();
	return r;
}

static uint16_t read12(uint8_t cmd)
{
	uint16_t raw16 = xpt_transfer(cmd);
	return (raw16 >> 4) & 0x0FFF;   // standard XPT2046/ADS7846 format
}

// median-of-3 (stabiliserer meget)
static uint16_t median3(uint16_t a, uint16_t b, uint16_t c)
{
	if (a>b){uint16_t t=a;a=b;b=t;}
	if (b>c){uint16_t t=b;b=c;c=t;}
	if (a>b){uint16_t t=a;a=b;b=t;}
	return b;
}

static uint16_t read_med3(uint8_t cmd)
{
	uint16_t a=read12(cmd), b=read12(cmd), c=read12(cmd);
	return median3(a,b,c);
}

void TouchInit(void)
{
	// outputs
	T_CLK_DDR |= (1<<T_CLK_PIN);
	T_CS_DDR  |= (1<<T_CS_PIN);
	T_DIN_DDR |= (1<<T_DIN_PIN);

	// inputs
	T_DOUT_DDR &= ~(1<<T_DOUT_PIN);
	T_IRQ_DDR  &= ~(1<<T_IRQ_PIN);

	// enable pull-ups (vigtigt hvis shieldet ikke har det)
	PORTE |= (1<<T_IRQ_PIN);   // PE4 pull-up (PENIRQ)
	PORTE |= (1<<T_DOUT_PIN);  // PE5 pull-up (ofte hjælper)

	// idle
	CS1();
	CLK0();      // mode 0 idle low (passer godt til datasheet timing)
	DIN0();

	// Optional: enable INT4 (falling edge)
	/*
	EICRB |= (1<<ISC41);
	EICRB &= ~(1<<ISC40);
	EIFR  |= (1<<INTF4);
	EIMSK |= (1<<INT4);
	sei();
	*/
}

bool TouchPressed(void)
{
	// PENIRQ active low
	return (IRQ() == 0);
}

bool TouchReadRaw(uint16_t *x, uint16_t *y)
{
	if (!x || !y) return false;
	if (!TouchPressed()) return false;

	// differential, 12-bit, PD=00 -> PENIRQ enabled after conversion
	// X=0xD0, Y=0x90 (typisk)
	uint16_t xr = read_med3(0xD0);
	uint16_t yr = read_med3(0x90);

	*x = xr;
	*y = yr;
	return true;
}

// simple map/clamp
static uint16_t map_u16(uint16_t v, uint16_t in_min, uint16_t in_max, uint16_t out_max)
{
	if (in_max <= in_min) return 0;     // vigtig!
	if (v < in_min) v = in_min;
	if (v > in_max) v = in_max;

	uint32_t num = (uint32_t)(v - in_min) * out_max;
	uint32_t den = (uint32_t)(in_max - in_min);
	return (uint16_t)(num / den);
}

bool TouchGetPoint(TouchPoint *p)
{
	if (!p) return false;

	uint16_t xr, yr;
	if (!TouchReadRaw(&xr, &yr)) return false;

	// Landscape: swap rå
	uint16_t rawX = yr;
	uint16_t rawY = xr;
	
	// KALIBRÉR DISSE efter rawX/rawY!
	const uint16_t xmin = 150;
	const uint16_t xmax = 3550;

	const uint16_t ymin = 150;
	const uint16_t ymax = 3500;

	uint16_t x = map_u16(rawX, xmin, xmax, 319);
	uint16_t y = map_u16(rawY, ymin, ymax, 239);

	// Fix retning (prøv én ad gangen)
	x = 319 - x;
	y = 239 - y;

	p->x = x;
	p->y = y;
	return true;
}