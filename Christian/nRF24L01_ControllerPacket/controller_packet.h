#ifndef CONTROLLER_PACKET_H
#define CONTROLLER_PACKET_H

#include <stdint.h>

#define CTRL_BTN_JOYSTICK   (1 << 0)
#define CTRL_FLAG_VALID     (1 << 0)

typedef struct {
	int8_t joyX;       // -127 .. 127
	int8_t joyY;       // -127 .. 127
	uint8_t buttons;   // bitfelt
	uint8_t flags;     // reserveret / valid
} ControllerPacket;

#endif