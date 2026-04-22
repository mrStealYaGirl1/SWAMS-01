#include "LeaderboardState.h"
#include "ConsoleContext.h"
#include "MenuState.h"
#include "TFTdriver.h"
#include "Joydriver.h"
#include "TextUI.h"

#define F_CPU 16000000UL
#include <util/delay.h>

static LeaderboardState leaderboardStateInstance;

static bool button_edge_leaderboard(void)
{
	static bool lastStable = false;
	JoyRaw jr;
	bool now;

	JoyReadRaw(&jr);
	now = jr.sw;

	if (now && !lastStable) {
		_delay_ms(15);
		JoyReadRaw(&jr);
		now = jr.sw;
		if (now) {
			lastStable = true;
			return true;
		}
	}

	if (!now && lastStable) {
		_delay_ms(10);
		JoyReadRaw(&jr);
		if (!jr.sw) {
			lastStable = false;
		}
	}

	return false;
}

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

LeaderboardState& LeaderboardState::Instance()
{
	return leaderboardStateInstance;
}

void LeaderboardState::Handle(ConsoleContext& context)
{
	if (context.stateJustChanged) {
		draw_leaderboard(context);
		context.stateJustChanged = false;
	}

	if (button_edge_leaderboard()) {
		context.SetState(&MenuState::Instance());
		_delay_ms(150);
		} else {
		_delay_ms(30);
	}
}