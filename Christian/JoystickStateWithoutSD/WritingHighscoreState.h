#pragma once
#include "State.h"

class WritingHighscoreState : public State {
	public:
	static WritingHighscoreState& Instance();
	void Handle(ConsoleContext& context);
};