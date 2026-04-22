/* 
* HighscoreStorage.cpp
*
* Created: 22-04-2026 09:19:22
* Author: jonathan
*/


#include "HighscoreStorage.h"
#include "SD_Driver.h"
#include <string.h>

#define HIGHSCORE_BLOCK 100
#define HIGHSCORE_MAGIC 0xAA

// Layout på SD (512 bytes)
typedef struct {
	uint8_t magic;
	uint8_t count;
	ScoreEntry entries[10];
	uint8_t padding[512 - 2 - sizeof(ScoreEntry) * 10];
} HighscoreBlock;


void HighscoreStorage_Init(ConsoleContext* context)
{
	// Du kan udvide senere hvis nødvendigt
	HighscoreStorage_Load(context);
}


void HighscoreStorage_Load(ConsoleContext* context)
{
	uint8_t buffer[512];

	if (SD_readSingleBlock(HIGHSCORE_BLOCK, buffer) != 0) {
		// Fejl ? reset
		context->recentScoreCount = 0;
		return;
	}

	HighscoreBlock* block = (HighscoreBlock*)buffer;

	if (block->magic != HIGHSCORE_MAGIC) {
		// Første gang ? reset
		context->recentScoreCount = 0;
		return;
	}

	context->recentScoreCount = block->count;

	if (context->recentScoreCount > 10) {
		context->recentScoreCount = 10;
	}

	memcpy(context->recentScores, block->entries, sizeof(ScoreEntry) * 10);
}


void HighscoreStorage_Save(ConsoleContext* context)
{
	uint8_t buffer[512];
	memset(buffer, 0xFF, sizeof(buffer));

	HighscoreBlock* block = (HighscoreBlock*)buffer;

	block->magic = HIGHSCORE_MAGIC;
	block->count = context->recentScoreCount;

	memcpy(block->entries, context->recentScores, sizeof(ScoreEntry) * 10);

	SD_writeSingleBlock(HIGHSCORE_BLOCK, buffer);
}