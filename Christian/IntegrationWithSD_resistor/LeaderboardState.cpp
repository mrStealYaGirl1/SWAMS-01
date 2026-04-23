#include "LeaderboardState.h"
#include "ConsoleContext.h"
#include "MenuState.h"
#include "TFTdriver.h"
#include "TextUI.h"
#include "ConsoleInput.h"

#include "HighscoreStorage.h"
#include "SD_Driver.h"
#include <avr/io.h>

#define F_CPU 16000000UL
#include <util/delay.h>

static LeaderboardState leaderboardStateInstance;

static void draw_index(uint16_t x, uint16_t y, uint8_t value)
{
	char s[4];

	if (value < 10) {
		s[0] = '0' + value;
		s[1] = '.';
		s[2] = '\0';
		} else {
		s[0] = '1';
		s[1] = '0';
		s[2] = '.';
		s[3] = '\0';
	}

	TextUI_DrawString(x, y, s, 31, 31, 31, 0, 0, 31, 1);
}

static void draw_leaderboard(ConsoleContext& context)
{
	uint8_t i;

	TFTFillRectangle(0, 0, 320, 240, 0, 0, 31);
	TextUI_DrawString(40, 10, "LEADERBOARD", 31, 31, 31, 0, 0, 31, 2);

	for (i = 0; i < context.recentScoreCount && i < 10; i++) {
		uint16_t y = 35 + i * 18;

		draw_index(10, y, i + 1);
		TextUI_DrawString(35, y, context.recentScores[i].name, 31, 31, 0, 0, 0, 31, 1);
		TextUI_DrawUInt16(90, y, context.recentScores[i].score, 31, 31, 31, 0, 0, 31, 1);
	}

	if (context.recentScoreCount == 0) {
		TextUI_DrawString(95, 110, "EMPTY", 31, 31, 31, 0, 0, 31, 2);
	}
}

static void draw_sd_error_screen(void)
{
	TFTFillRectangle(0, 0, 320, 240, 0, 0, 0);
	TextUI_DrawString(70, 90, "SD ERROR", 31, 0, 0, 0, 0, 0, 3);
}

static void Radio_DisableForSD(void)
{
	PORTL |= (1 << PL6);   // nRF CSN HIGH
	PORTL &= ~(1 << PL7);  // nRF CE LOW
}

static void Radio_EnableRX(void)
{
	PORTB |= (1 << PB0);   // SD_CS HIGH = SD inactive
	PORTL |= (1 << PL6);   // nRF CSN HIGH = SPI idle
	PORTL |= (1 << PL7);   // nRF CE HIGH = RX active
	_delay_us(200);        // RX settling
}

LeaderboardState& LeaderboardState::Instance()
{
	return leaderboardStateInstance;
}

void LeaderboardState::Handle(ConsoleContext& context)
{
	if (context.stateJustChanged) {
		Radio_DisableForSD();

		if (SD_init() == 0) {
			HighscoreStorage_Load(&context);
			draw_leaderboard(context);
			} else {
			draw_sd_error_screen();
		}

		// Give TFT time to keep showing the loaded leaderboard
		_delay_ms(2000);

		// Reactivate radio after SD access is done
		Radio_EnableRX();

		context.stateJustChanged = false;
	}

	if (ConsoleInput_ButtonEdge()) {
		context.SetState(&MenuState::Instance());
		_delay_ms(150);
		} else {
		_delay_ms(30);
	}
}