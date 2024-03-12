/* control.h
 *
*/
#ifndef CONTROL_LIB_H
#define CONTROL_LIB_H

#include "spdlog/spdlog.h"
#include "../cv_lib/computer_vision.h"
#include <unistd.h>
#include <stdio.h>
#include <linux/joystick.h>
#include <thread>
#include <fcntl.h>
#include <signal.h>
#include <wiringPi.h>


#define PREV_STATE -1
#define NEXT_STATE 1

//Joystick buttons and sticks (axis)
#define BTN_TOGGLE_MODE 6
#define BTN_BEEP 2
#define AXIS_HORZONTAL 0
#define AXIS_VERTICAL 4
#define AXIS_SPOOL 2
#define AXIS_FIRE 5

//Maximums and Minimums
#define MAX_JSTICK 32767
#define MAX_SERVO 250
#define MIN_SERVO 230

//Global
extern bool RUN_MANUAL;

//Public Functions
int RunIdle();
int RunObjDetect();
int RunTargetWarn();
int RunTargetFire();

//Constructor and destructor
void InitCL(std::shared_ptr<spdlog::logger> logger);
void DestructCL();

#endif