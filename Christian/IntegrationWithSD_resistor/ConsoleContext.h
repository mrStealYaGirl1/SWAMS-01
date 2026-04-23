#pragma once
#include <stdint.h>

class State;

struct ScoreEntry {
	char name[5];
	uint16_t score;
};

class ConsoleContext {
	private:
	State* state;

	public:
	ConsoleContext();

	void SetState(State* newState);
	void Request();

	State* GetState();

	uint8_t menuIndex;
	bool paused;
	bool stateJustChanged;
	uint16_t lastScore;

	// Highscore input
	char playerName[5];
	uint8_t namePos;

	// Top 10 highscores i RAM
	ScoreEntry recentScores[10];
	uint8_t recentScoreCount;
};