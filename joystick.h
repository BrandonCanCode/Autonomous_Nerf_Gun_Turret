/* joystick.h
*/
#include <linux/joystick.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "control.h"

//Joystick buttons and sticks (axis)
#define BTN_TOGGLE_MODE 6
#define BTN_BEEP 2
#define AXIS_HORIZONTAL 0
#define AXIS_VERTICAL 4
#define AXIS_SPOOL 2
#define AXIS_FIRE 5
#define DEAD_ZONE 10

//Shared globals
extern bool RUN_MANUAL;

void InitJS();
void DestructJS();