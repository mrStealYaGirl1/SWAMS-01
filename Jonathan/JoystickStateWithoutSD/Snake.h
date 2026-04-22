#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Dir;

void Snake_init(void);
bool Snake_step(void);
void Snake_setDirection(Dir d);
void Snake_draw(void);
uint16_t Snake_getScore(void);