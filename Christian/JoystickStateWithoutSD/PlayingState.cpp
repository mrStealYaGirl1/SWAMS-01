#include "PlayingState.h"
#include "ConsoleContext.h"
#include "PausedState.h"
#include "WritingHighscoreState.h"
#include "Snake.h"
#include "Joydriver.h"

#define F_CPU 16000000UL
#include <util/delay.h>

static PlayingState playingStateInstance;

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

PlayingState& PlayingState::Instance()
{
	return playingStateInstance;
}

void PlayingState::Handle(ConsoleContext& context)
{
	Dir d;
	bool alive;

	if (context.stateJustChanged) {
		Snake_init();
		context.paused = false;
		context.stateJustChanged = false;
	}

	if (button_edge_playing()) {
		context.paused = true;
		context.SetState(&PausedState::Instance());
		_delay_ms(150);
		return;
	}

	if (joy_to_dir(&d)) {
		Snake_setDirection(d);
	}

	alive = Snake_step();
	if (!alive) {
		context.lastScore = Snake_getScore();
		context.SetState(&WritingHighscoreState::Instance());
		_delay_ms(150);
		return;
	}

	_delay_ms(120);
}