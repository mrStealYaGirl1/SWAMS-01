#include "ConsoleContext.h"
#include "State.h"

ConsoleContext::ConsoleContext()
: state(0), menuIndex(0), paused(false), stateJustChanged(false),
lastScore(0), namePos(0), recentScoreCount(0)
{
	uint8_t i, j;

	playerName[0] = 'A';
	playerName[1] = 'A';
	playerName[2] = 'A';
	playerName[3] = 'A';
	playerName[4] = '\0';

	for (i = 0; i < 10; i++) {
		for (j = 0; j < 4; j++) {
			recentScores[i].name[j] = '-';
		}
		recentScores[i].name[4] = '\0';
		recentScores[i].score = 0;
	}
}

void ConsoleContext::SetState(State* newState)
{
	state = newState;
	stateJustChanged = true;
}

void ConsoleContext::Request()
{
	if (state != 0) {
		state->Handle(*this);
	}
}

State* ConsoleContext::GetState()
{
	return state;
}