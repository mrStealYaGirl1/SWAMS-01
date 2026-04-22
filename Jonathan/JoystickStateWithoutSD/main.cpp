#include "TFTdriver.h"
#include "Joydriver.h"
#include "ConsoleContext.h"
#include "MenuState.h"
#include "SD_Driver.h"
#include "HighscoreStorage.h"

#include <stdlib.h>

int main(void)
{
	TFTDisplayInit();
	JoyInit();

	srand(1234);

	ConsoleContext context;              // ? opret først
	context.SetState(&MenuState::Instance());

	SD_init();                          // init SD
	HighscoreStorage_Init(&context);    // load highscores

	while (1) {
		context.Request();
	}
}