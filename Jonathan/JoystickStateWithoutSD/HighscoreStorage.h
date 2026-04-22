/* 
* HighscoreStorage.h
*
* Created: 22-04-2026 09:19:23
* Author: jonathan
*/


#pragma once

#include <stdint.h>
#include "ConsoleContext.h"

// Init (kaldes efter SD_init)
void HighscoreStorage_Init(ConsoleContext* context);

// Load fra SD ? RAM (context)
void HighscoreStorage_Load(ConsoleContext* context);

// Save fra RAM ? SD
void HighscoreStorage_Save(ConsoleContext* context);