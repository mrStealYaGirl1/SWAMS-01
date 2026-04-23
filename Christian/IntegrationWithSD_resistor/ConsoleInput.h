#ifndef CONSOLE_INPUT_H
#define CONSOLE_INPUT_H

#include <stdbool.h>
#include <stdint.h>
#include "Joydriver.h"
#include "controller_packet.h"

void ConsoleInput_Init(void);
void ConsoleInput_Update(void);

JoyDir ConsoleInput_GetDir(void);
bool ConsoleInput_ButtonDown(void);
bool ConsoleInput_ButtonEdge(void);

bool ConsoleInput_HasValidPacket(void);
const ControllerPacket* ConsoleInput_GetPacket(void);

#endif