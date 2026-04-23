#include "ConsoleInput.h"
#include "nrf24.h"

static ControllerPacket currentPacket;
static bool hasValidPacket = false;

static bool currentButton = false;
static bool previousButton = false;
static bool buttonEdge = false;

static JoyDir packet_to_dir(const ControllerPacket* pkt)
{
	const int8_t threshold = 50;

	if (!pkt) return JOY_NONE;
	if (!(pkt->flags & CTRL_FLAG_VALID)) return JOY_NONE;

	if (pkt->joyX > threshold)  return JOY_RIGHT;
	if (pkt->joyX < -threshold) return JOY_LEFT;
	if (pkt->joyY > threshold)  return JOY_DOWN;
	if (pkt->joyY < -threshold) return JOY_UP;

	return JOY_NONE;
}

void ConsoleInput_Init(void)
{
	currentPacket.joyX = 0;
	currentPacket.joyY = 0;
	currentPacket.buttons = 0;
	currentPacket.flags = 0;

	hasValidPacket = false;
	currentButton = false;
	previousButton = false;
	buttonEdge = false;
}

void ConsoleInput_Update(void)
{
	if (NRF24DataReady()) {
		NRF24ReceivePacket((uint8_t*)&currentPacket, sizeof(ControllerPacket));
		hasValidPacket = ((currentPacket.flags & CTRL_FLAG_VALID) != 0);
	}

	previousButton = currentButton;

	if (hasValidPacket) {
		currentButton = ((currentPacket.buttons & CTRL_BTN_JOYSTICK) != 0);
		} else {
		currentButton = false;
	}

	buttonEdge = (currentButton && !previousButton);
}

JoyDir ConsoleInput_GetDir(void)
{
	if (!hasValidPacket) return JOY_NONE;
	return packet_to_dir(&currentPacket);
}

bool ConsoleInput_ButtonDown(void)
{
	return currentButton;
}

bool ConsoleInput_ButtonEdge(void)
{
	return buttonEdge;
}

bool ConsoleInput_HasValidPacket(void)
{
	return hasValidPacket;
}

const ControllerPacket* ConsoleInput_GetPacket(void)
{
	return &currentPacket;
}