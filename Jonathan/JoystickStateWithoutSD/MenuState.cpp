#include "MenuState.h"
#include "ConsoleContext.h"
#include "PlayingState.h"
#include "LeaderboardState.h"
#include "Joydriver.h"
#include "TFTdriver.h"

#include "TextUI.h"

#define F_CPU 16000000UL
#include <util/delay.h>

static MenuState menuStateInstance;

static bool button_edge_menu(void)
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

static int menu_dir(void)
{
	JoyRaw jr;
	JoyDir jd;

	if (!JoyReadRaw(&jr)) return 0;
	jd = JoyGetDirDefault(jr.x, jr.y);

	if (jd == JOY_UP) return -1;
	if (jd == JOY_DOWN) return 1;
	return 0;
}

static void draw_menu(uint8_t index)
{
	TFTFillRectangle(0, 0, 320, 240, 0, 0, 0);

	// PLAY box
	if (index == 0) TFTFillRectangle(30, 40, 260, 60, 0, 63, 0);
	else            TFTFillRectangle(30, 40, 260, 60, 0, 20, 0);

	// LEADERBOARD box
	if (index == 1) TFTFillRectangle(30, 130, 260, 60, 0, 0, 31);
	else            TFTFillRectangle(30, 130, 260, 60, 0, 0, 10);

	TextUI_DrawString(110, 60, "SPIL", 31, 31, 31, 0, 63, 0, 3);
	TextUI_DrawString(58, 150, "LEADERBOARD", 31, 31, 31, 0, 0, 31, 2);
}

MenuState& MenuState::Instance()
{
	return menuStateInstance;
}

void MenuState::Handle(ConsoleContext& context)
{
	int d;

	if (context.stateJustChanged) {
		context.menuIndex = 0;
		draw_menu(context.menuIndex);
		context.stateJustChanged = false;
	}

	d = menu_dir();

	if (d < 0 && context.menuIndex > 0) {
		context.menuIndex--;
		draw_menu(context.menuIndex);
		_delay_ms(180);
		} else if (d > 0 && context.menuIndex < 1) {
		context.menuIndex++;
		draw_menu(context.menuIndex);
		_delay_ms(180);
	}

	if (button_edge_menu()) {
		if (context.menuIndex == 0) {
			context.SetState(&PlayingState::Instance());
			} else {
			context.SetState(&LeaderboardState::Instance());
		}
		_delay_ms(150);
	}
}