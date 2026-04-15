#pragma once
#include "State.h"

class LeaderboardState : public State {
	public:
	static LeaderboardState& Instance();
	void Handle(ConsoleContext& context);
};