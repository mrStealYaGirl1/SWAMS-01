#include "TFTdriver.h"
#include "Joydriver.h"

#define F_CPU 16000000UL
#include <util/delay.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Dir;

static const uint8_t CELL = 16;
static const uint8_t GRID_W = 320 / CELL; // 20
static const uint8_t GRID_H = 240 / CELL; // 15
static const uint16_t MAX_SNAKE = GRID_W * GRID_H;

typedef struct { uint8_t x, y; } Pt;

static Pt snake[MAX_SNAKE];
static uint16_t snakeLen;

static Pt food;
static Dir dir, nextDir;
static bool paused = false;

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
	for (uint16_t i = 0; i < snakeLen; i++) {
		if (snake[i].x == x && snake[i].y == y) return true;
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

static void game_init(void)
{
	clear_screen();

	snakeLen = 3;
	snake[0] = (Pt){ GRID_W / 2, GRID_H / 2 };
	snake[1] = (Pt){ (uint8_t)(GRID_W / 2 - 1), GRID_H / 2 };
	snake[2] = (Pt){ (uint8_t)(GRID_W / 2 - 2), GRID_H / 2 };

	dir = DIR_RIGHT;
	nextDir = dir;
	paused = false;

	spawn_food();

	for (uint16_t i = 0; i < snakeLen; i++) {
		draw_cell(snake[i].x, snake[i].y, 0, 63, 0);
	}

	draw_cell(food.x, food.y, 31, 0, 0);
}

static bool step(void)
{
	if (!opposite(nextDir, dir)) {
		dir = nextDir;
	}

	int nx = snake[0].x;
	int ny = snake[0].y;

	if (dir == DIR_RIGHT)      nx++;
	else if (dir == DIR_LEFT)  nx--;
	else if (dir == DIR_UP)    ny--;
	else if (dir == DIR_DOWN)  ny++;

	if (nx < 0 || ny < 0 || nx >= GRID_W || ny >= GRID_H) return false;
	if (cell_occupied((uint8_t)nx, (uint8_t)ny)) return false;

	Pt newHead = { (uint8_t)nx, (uint8_t)ny };
	bool ate = (newHead.x == food.x && newHead.y == food.y);

	if (!ate) {
		Pt tail = snake[snakeLen - 1];
		draw_cell(tail.x, tail.y, 0, 0, 0);
		} else {
		if (snakeLen < MAX_SNAKE) snakeLen++;
	}

	for (int i = (int)snakeLen - 1; i > 0; i--) {
		snake[i] = snake[i - 1];
	}
	snake[0] = newHead;

	draw_cell(snake[0].x, snake[0].y, 0, 63, 0);

	if (ate) {
		spawn_food();
		draw_cell(food.x, food.y, 31, 0, 0);
	}

	return true;
}

static bool joy_to_dir(Dir *outDir)
{
	JoyRaw jr;
	if (!JoyReadRaw(&jr)) return false;

	JoyDir jd = JoyGetDirDefault(jr.x, jr.y);

	switch (jd) {
		case JOY_UP:
		*outDir = DIR_UP;
		return true;
		case JOY_DOWN:
		*outDir = DIR_DOWN;
		return true;
		case JOY_LEFT:
		*outDir = DIR_LEFT;
		return true;
		case JOY_RIGHT:
		*outDir = DIR_RIGHT;
		return true;
		case JOY_NONE:
		default:
		return false;
	}
}

// true én gang pr. fysisk tryk
static bool joystick_button_pressed_edge(void)
{
	static bool lastStable = false;

	JoyRaw jr;
	JoyReadRaw(&jr);
	bool now = jr.sw;

	if (now && !lastStable) {
		_delay_ms(15);   // debounce
		JoyReadRaw(&jr);
		now = jr.sw;

		if (now) {
			lastStable = true;
			return true;
		}
	}

	if (!now && lastStable) {
		_delay_ms(10);
		JoyReadRaw(&jr);
		if (!jr.sw) {
			lastStable = false;
		}
	}

	return false;
}

int main(void)
{
	TFTDisplayInit();
	JoyInit();

	srand(1234);
	game_init();

	while (1)
	{
		// toggle pause på joystick-knap
		if (joystick_button_pressed_edge()) {
			paused = !paused;

			// lille pause-indikator øverst
			if (paused) {
				TFTFillRectangle(0, 0, 320, 10, 31, 63, 0);
				} else {
				TFTFillRectangle(0, 0, 320, 10, 0, 0, 0);
			}
		}

		// retning må gerne læses selv om spillet er paused
		Dir d;
		if (joy_to_dir(&d)) {
			nextDir = d;
		}

		if (!paused) {
			bool alive = step();
			if (!alive) {
				TFTFillRectangle(0, 0, 320, 240, 0, 0, 31);
				_delay_ms(800);
				game_init();
			}

			_delay_ms(120);
			} else {
			_delay_ms(30);
		}
	}
}