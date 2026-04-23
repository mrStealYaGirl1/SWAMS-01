#include "WritingHighscoreState.h"
#include "ConsoleContext.h"
#include "MenuState.h"
#include "TFTdriver.h"
#include "TextUI.h"
#include "ConsoleInput.h"

#include "HighscoreStorage.h"
#include "SD_Driver.h"
#include <avr/io.h>

#define F_CPU 16000000UL
#include <util/delay.h>

static WritingHighscoreState writingHighscoreStateInstance;

static int input_vertical(void)
{
	JoyDir jd = ConsoleInput_GetDir();

	if (jd == JOY_UP) return -1;
	if (jd == JOY_DOWN) return 1;
	return 0;
}

static void draw_name_ui(ConsoleContext& context)
{
	char buf[5];
	uint8_t i;

	for (i = 0; i < 4; i++) {
		buf[i] = context.playerName[i];
	}
	buf[4] = '\0';

	TFTFillRectangle(0, 0, 320, 240, 31, 0, 0);

	TextUI_DrawString(20, 20, "GAME OVER", 31, 31, 31, 31, 0, 0, 3);
	TextUI_DrawString(20, 70, "SCORE =", 31, 31, 31, 31, 0, 0, 2);
	TextUI_DrawUInt16(120, 70, context.lastScore, 0, 0, 31, 31, 0, 0, 2);

	TextUI_DrawString(20, 110, "NAME:", 31, 31, 31, 31, 0, 0, 2);
	TextUI_DrawString(120, 110, buf, 31, 31, 31, 31, 0, 0, 3);

	if (context.namePos < 4) {
		TFTFillRectangle(120 + context.namePos * (6 * 3), 135, 15, 4, 0, 0, 31);
		} else {
		TFTFillRectangle(110, 170, 70, 30, 0, 63, 0);
		TextUI_DrawString(125, 178, "OK", 31, 31, 31, 0, 63, 0, 2);
	}

	if (context.namePos < 4) {
		TFTFillRectangle(110, 170, 70, 30, 10, 10, 10);
		TextUI_DrawString(125, 178, "OK", 31, 31, 31, 10, 10, 10, 2);
	}
}

static void draw_saved_screen(void)
{
	TFTFillRectangle(0, 0, 320, 240, 0, 0, 0);
	TextUI_DrawString(95, 100, "SAVED", 0, 63, 0, 0, 0, 0, 3);
}

static void draw_sd_error_screen(void)
{
	TFTFillRectangle(0, 0, 320, 240, 0, 0, 0);
	TextUI_DrawString(70, 100, "SD ERROR", 31, 0, 0, 0, 0, 0, 3);
}

static void insertScore(ConsoleContext& context, const char* name, uint16_t score)
{
	int8_t i;
	int8_t pos = -1;

	for (i = 0; i < context.recentScoreCount; i++) {
		if (score > context.recentScores[i].score) {
			pos = i;
			break;
		}
	}

	if (pos == -1) {
		if (context.recentScoreCount < 10) {
			pos = context.recentScoreCount;
			} else {
			return;
		}
	}

	if (context.recentScoreCount < 10) {
		context.recentScoreCount++;
	}

	for (i = context.recentScoreCount - 1; i > pos; i--) {
		context.recentScores[i] = context.recentScores[i - 1];
	}

	for (i = 0; i < 4; i++) {
		context.recentScores[pos].name[i] = name[i];
	}
	context.recentScores[pos].name[4] = '\0';
	context.recentScores[pos].score = score;
}

WritingHighscoreState& WritingHighscoreState::Instance()
{
	return writingHighscoreStateInstance;
}

void WritingHighscoreState::Handle(ConsoleContext& context)
{
	int v;

	if (context.stateJustChanged) {
		context.playerName[0] = 'A';
		context.playerName[1] = 'A';
		context.playerName[2] = 'A';
		context.playerName[3] = 'A';
		context.playerName[4] = '\0';
		context.namePos = 0;

		draw_name_ui(context);
		context.stateJustChanged = false;
	}

	v = input_vertical();

	if (context.namePos < 4) {
		if (v < 0) {
			if (context.playerName[context.namePos] == 'A')
			context.playerName[context.namePos] = 'Z';
			else
			context.playerName[context.namePos]--;

			draw_name_ui(context);
			_delay_ms(180);
			return;
		}
		else if (v > 0) {
			if (context.playerName[context.namePos] == 'Z')
			context.playerName[context.namePos] = 'A';
			else
			context.playerName[context.namePos]++;

			draw_name_ui(context);
			_delay_ms(180);
			return;
		}
	}

	if (ConsoleInput_ButtonEdge()) {
		if (context.namePos < 4) {
			context.namePos++;
			draw_name_ui(context);
			_delay_ms(150);
			return;
			} else {
			// Make sure nRF is inactive while SD is used
			PORTL |= (1 << PL6);   // nRF CSN HIGH
			PORTL &= ~(1 << PL7);  // nRF CE LOW

			if (SD_init() == 0) {
				// SD is source of truth
				HighscoreStorage_Load(&context);
				insertScore(context, context.playerName, context.lastScore);
				HighscoreStorage_Save(&context);

				draw_saved_screen();
				_delay_ms(1000);
				} else {
				draw_sd_error_screen();
				_delay_ms(1500);
			}

			// Deactivate SD again
			PORTB |= (1 << PB0);   // SD_CS HIGH

			// Reactivate nRF RX
			PORTL |= (1 << PL6);   // nRF CSN HIGH (idle on SPI)
			PORTL |= (1 << PL7);   // nRF CE HIGH -> listen in RX mode
			_delay_us(200);
			
			context.SetState(&MenuState::Instance());
			_delay_ms(150);
			return;
		}
	}

	_delay_ms(30);
}