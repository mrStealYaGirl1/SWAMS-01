#include "Snake.h"
#include "TFTdriver.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

static const uint8_t CELL = 16;
static const uint8_t GRID_W = 320 / CELL;
static const uint8_t GRID_H = 240 / CELL;
static const uint16_t MAX_SNAKE = GRID_W * GRID_H;

typedef struct {
	uint8_t x;
	uint8_t y;
} Pt;

static Pt snake[MAX_SNAKE];
static uint16_t snakeLen;
static Pt food;
static Dir dir, nextDir;
static uint16_t score;

static void draw_cell(uint8_t gx, uint8_t gy, uint8_t r, uint8_t g, uint8_t b)
{
	TFTFillRectangle((uint16_t)gx * CELL, (uint16_t)gy * CELL, CELL, CELL, r, g, b);
}

static void clear_screen(void)
{
	TFTFillRectangle(0, 0, 320, 240, 0, 0, 0);
}

static bool cell_occupied(uint8_t x, uint8_t y)
{
	uint16_t i;
	for (i = 0; i < snakeLen; i++) {
		if (snake[i].x == x && snake[i].y == y) {
			return true;
		}
	}
	return false;
}

static void spawn_food(void)
{
	while (1) {
		uint8_t x = (uint8_t)(rand() % GRID_W);
		uint8_t y = (uint8_t)(rand() % GRID_H);

		if (!cell_occupied(x, y)) {
			food.x = x;
			food.y = y;
			return;
		}
	}
}

static bool opposite(Dir a, Dir b)
{
	return (a == DIR_UP    && b == DIR_DOWN)  ||
	(a == DIR_DOWN  && b == DIR_UP)    ||
	(a == DIR_LEFT  && b == DIR_RIGHT) ||
	(a == DIR_RIGHT && b == DIR_LEFT);
}

void Snake_draw(void)
{
	uint16_t i;

	clear_screen();

	for (i = 0; i < snakeLen; i++) {
		draw_cell(snake[i].x, snake[i].y, 0, 63, 0);
	}

	draw_cell(food.x, food.y, 31, 0, 0);
}

void Snake_init(void)
{
	clear_screen();

	snakeLen = 3;
	score = 0;

	snake[0].x = GRID_W / 2;
	snake[0].y = GRID_H / 2;

	snake[1].x = (uint8_t)(GRID_W / 2 - 1);
	snake[1].y = GRID_H / 2;

	snake[2].x = (uint8_t)(GRID_W / 2 - 2);
	snake[2].y = GRID_H / 2;

	dir = DIR_RIGHT;
	nextDir = DIR_RIGHT;

	spawn_food();
	Snake_draw();
}

void Snake_setDirection(Dir d)
{
	nextDir = d;
}

bool Snake_step(void)
{
	int nx = snake[0].x;
	int ny = snake[0].y;
	Pt newHead;
	bool ate;

	if (!opposite(nextDir, dir)) {
		dir = nextDir;
	}

	if (dir == DIR_RIGHT) nx++;
	else if (dir == DIR_LEFT) nx--;
	else if (dir == DIR_UP) ny--;
	else if (dir == DIR_DOWN) ny++;

	if (nx < 0 || ny < 0 || nx >= GRID_W || ny >= GRID_H) {
		return false;
	}

	if (cell_occupied((uint8_t)nx, (uint8_t)ny)) {
		return false;
	}

	newHead.x = (uint8_t)nx;
	newHead.y = (uint8_t)ny;

	ate = (newHead.x == food.x && newHead.y == food.y);

	if (!ate) {
		Pt tail = snake[snakeLen - 1];
		draw_cell(tail.x, tail.y, 0, 0, 0);
		} else {
		if (snakeLen < MAX_SNAKE) {
			snakeLen++;
		}
		score++;
	}

	{
		int i;
		for (i = (int)snakeLen - 1; i > 0; i--) {
			snake[i] = snake[i - 1];
		}
	}

	snake[0] = newHead;
	draw_cell(snake[0].x, snake[0].y, 0, 63, 0);

	if (ate) {
		spawn_food();
		draw_cell(food.x, food.y, 31, 0, 0);
	}

	return true;
}

uint16_t Snake_getScore(void)
{
	return score;
}