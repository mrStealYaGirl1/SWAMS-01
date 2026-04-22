#pragma once
#include "State.h"

class PausedState : public State {
	public:
	static PausedState& Instance();
	void Handle(ConsoleContext& context);
};