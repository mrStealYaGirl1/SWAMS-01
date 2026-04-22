#pragma once

class ConsoleContext;

class State {
public:
	virtual void Handle(ConsoleContext& context) = 0;
};