#include "TFTdriver.h"
#include "Joydriver.h"
#include "ConsoleContext.h"
#include "MenuState.h"

#include <stdlib.h>

int main(void)
{
	TFTDisplayInit();
	JoyInit();

	srand(1234);

	ConsoleContext context;
	context.SetState(&MenuState::Instance());

	while (1) {
		context.Request();
	}
}