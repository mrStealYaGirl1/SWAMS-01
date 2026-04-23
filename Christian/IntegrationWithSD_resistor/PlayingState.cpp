#include "PlayingState.h"
#include "ConsoleContext.h"
#include "PausedState.h"
#include "WritingHighscoreState.h"
#include "Snake.h"
//#include "Joydriver.h"

#include "ConsoleInput.h"

#define F_CPU 16000000UL
#include <util/delay.h>

static PlayingState playingStateInstance;

static uint16_t gameTickCounter = 0;

/*
static bool button_edge_playing(void)
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

static bool joy_to_dir(Dir* outDir)
{
	JoyRaw jr;
	JoyDir jd;

	if (!JoyReadRaw(&jr)) return false;

	jd = JoyGetDirDefault(jr.x, jr.y);

	switch (jd) {
		case JOY_UP:    *outDir = DIR_UP;    return true;
		case JOY_DOWN:  *outDir = DIR_DOWN;  return true;
		case JOY_LEFT:  *outDir = DIR_LEFT;  return true;
		case JOY_RIGHT: *outDir = DIR_RIGHT; return true;
		default: return false;
	}
}
*/

PlayingState& PlayingState::Instance()
{
	return playingStateInstance;
}

void PlayingState::Handle(ConsoleContext& context)
{
	JoyDir jd;
	Dir d;
	bool alive;

	if (context.stateJustChanged) {
		Snake_init();
		context.paused = false;
		gameTickCounter = 0;
		context.stateJustChanged = false;
	}

	if (ConsoleInput_ButtonEdge()) {
		context.paused = true;
		context.SetState(&PausedState::Instance());
		_delay_ms(150);
		return;
	}

	jd = ConsoleInput_GetDir();

	switch (jd) {
		case JOY_UP:    d = DIR_UP;    Snake_setDirection(d); break;
		case JOY_DOWN:  d = DIR_DOWN;  Snake_setDirection(d); break;
		case JOY_LEFT:  d = DIR_LEFT;  Snake_setDirection(d); break;
		case JOY_RIGHT: d = DIR_RIGHT; Snake_setDirection(d); break;
		default: break;
	}

	if (gameTickCounter >= 10) {   // 10 * 10 ms = 100 ms
		alive = Snake_step();
		gameTickCounter = 0;

		if (!alive) {
			context.lastScore = Snake_getScore();
			context.SetState(&WritingHighscoreState::Instance());
			_delay_ms(150);
			return;
		}
		} else {
		gameTickCounter++;
	}

	_delay_ms(10);
}