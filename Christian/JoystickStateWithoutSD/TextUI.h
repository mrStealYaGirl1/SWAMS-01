#pragma once
#include <stdint.h>

void TextUI_DrawChar(uint16_t x, uint16_t y, char c,
uint8_t r, uint8_t g, uint8_t b,
uint8_t bgR, uint8_t bgG, uint8_t bgB,
uint8_t scale);

void TextUI_DrawString(uint16_t x, uint16_t y, const char* s,
uint8_t r, uint8_t g, uint8_t b,
uint8_t bgR, uint8_t bgG, uint8_t bgB,
uint8_t scale);

void TextUI_DrawUInt16(uint16_t x, uint16_t y, uint16_t value,
uint8_t r, uint8_t g, uint8_t b,
uint8_t bgR, uint8_t bgG, uint8_t bgB,
uint8_t scale);