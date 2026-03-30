#include "TFTdriver.h"
#include "Touchdriver.h"

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

typedef struct {
	uint8_t x, y;
} Pt;

static Pt snake[MAX_SNAKE];
static uint16_t snakeLen;

static Pt food;
static Dir dir, nextDir;

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
	// naive: prøv indtil fri (ok til små grids)
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

// Din threshold-baserede decode (samme idé som du har nu)
static Dir decode_dir_from_raw(uint16_t xr, uint16_t yr, bool *valid)
{
	*valid = true;

	uint16_t rawX = yr;  // LEFT/RIGHT
	uint16_t rawY = xr;  // UP/DOWN

	const uint16_t X_RIGHT_MAX = 650;
	const uint16_t X_LEFT_MIN  = 1420;

	const uint16_t Y_DOWN_MAX  = 670;
	const uint16_t Y_UP_MIN    = 1350;

	if (rawX < X_RIGHT_MAX) return DIR_RIGHT;
	if (rawX > X_LEFT_MIN)  return DIR_LEFT;

	if (rawY > Y_UP_MIN)    return DIR_UP;
	if (rawY < Y_DOWN_MAX)  return DIR_DOWN;

	*valid = false; // deadzone
	return DIR_RIGHT; // dummy
}

static bool opposite(Dir a, Dir b)
{
	return (a == DIR_UP && b == DIR_DOWN) ||
	(a == DIR_DOWN && b == DIR_UP) ||
	(a == DIR_LEFT && b == DIR_RIGHT) ||
	(a == DIR_RIGHT && b == DIR_LEFT);
}

static void game_init(void)
{
	clear_screen();

	// start snake midt på
	snakeLen = 3;
	snake[0] = (Pt){ GRID_W / 2, GRID_H / 2 };
	snake[1] = (Pt){ (uint8_t)(GRID_W / 2 - 1), GRID_H / 2 };
	snake[2] = (Pt){ (uint8_t)(GRID_W / 2 - 2), GRID_H / 2 };

	dir = DIR_RIGHT;
	nextDir = dir;

	spawn_food();

	// tegn initial state
	for (uint16_t i = 0; i < snakeLen; i++) {
		draw_cell(snake[i].x, snake[i].y, 0, 63, 0); // grøn snake
	}
	draw_cell(food.x, food.y, 31, 0, 0); // rød food
}

static bool step(void)
{
	// apply buffered direction
	if (!opposite(nextDir, dir)) dir = nextDir;

	// ny head pos
	int nx = snake[0].x;
	int ny = snake[0].y;

	if (dir == DIR_RIGHT)      nx++;
	else if (dir == DIR_LEFT)  nx--;
	else if (dir == DIR_UP)    ny--;
	else if (dir == DIR_DOWN)  ny++;

	// wall collision
	if (nx < 0 || ny < 0 || nx >= GRID_W || ny >= GRID_H) return false;

	// self collision
	if (cell_occupied((uint8_t)nx, (uint8_t)ny)) return false;

	// flyt kroppen: tegn kun diff (hurtigt)
	Pt newHead = { (uint8_t)nx, (uint8_t)ny };

	bool ate = (newHead.x == food.x && newHead.y == food.y);

	// hvis ikke ate: slet tail-pixel
	if (!ate) {
		Pt tail = snake[snakeLen - 1];
		draw_cell(tail.x, tail.y, 0, 0, 0);
		} else {
		if (snakeLen < MAX_SNAKE) snakeLen++;
	}

	// shift array ned
	for (int i = (int)snakeLen - 1; i > 0; i--) {
		snake[i] = snake[i - 1];
	}
	snake[0] = newHead;

	// tegn head
	draw_cell(snake[0].x, snake[0].y, 0, 63, 0);

	if (ate) {
		spawn_food();
		draw_cell(food.x, food.y, 31, 0, 0);
	}

	return true;
}

int main(void)
{
	TFTDisplayInit();
	TouchInit();

	// seed (for test kan du også bare srand(1);)
	srand(1234);

	game_init();

	while (1)
	{
		// input: touch -> retning
		if (TouchPressed()) {
			uint16_t xr, yr;
			if (TouchReadRaw(&xr, &yr)) {
				bool valid;
				Dir d = decode_dir_from_raw(xr, yr, &valid);
				if (valid) nextDir = d;
			}
		}

		// game tick
		bool alive = step();
		if (!alive) {
			// game over: fyld skærm blå
			TFTFillRectangle(0, 0, 320, 240, 0, 0, 31);
			_delay_ms(800);
			game_init();
		}

		_delay_ms(120); // speed (justér)
	}
}