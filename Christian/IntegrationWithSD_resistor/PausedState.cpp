#include "PausedState.h"
#include "ConsoleContext.h"
#include "PlayingState.h"
#include "TFTdriver.h"
//#include "Joydriver.h"

#include "ConsoleInput.h"

#include "TextUI.h"
#include "Snake.h"

#define F_CPU 16000000UL
#include <util/delay.h>

static PausedState pausedStateInstance;

/*
static bool button_edge_paused(void)
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
*/
PausedState& PausedState::Instance()
{
	return pausedStateInstance;
}

void PausedState::Handle(ConsoleContext& context)
{
	if (context.stateJustChanged) {
		TFTFillRectangle(70, 95, 180, 50, 0, 0, 0);
		TextUI_DrawString(95, 110, "PAUSED", 31, 31, 0, 0, 0, 0, 3);
		context.stateJustChanged = false;
	}

	if (ConsoleInput_ButtonEdge()) {
		Snake_draw();
		context.paused = false;
		context.SetState(&PlayingState::Instance());
		_delay_ms(150);
		} else {
		_delay_ms(30);
	}
}