#pragma once
#include "State.h"

class PlayingState : public State {
	public:
	static PlayingState& Instance();
	void Handle(ConsoleContext& context);
};