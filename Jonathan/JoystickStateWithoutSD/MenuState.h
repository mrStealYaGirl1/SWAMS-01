#pragma once
#include "State.h"

class MenuState : public State {
	public:
	static MenuState& Instance();
	void Handle(ConsoleContext& context);
};