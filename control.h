/* control.h
 *
*/
#ifndef CONTROL_LIB_H
#define CONTROL_LIB_H

#include "spdlog/spdlog.h"
#include <unistd.h>
#include <stdio.h>
#include <linux/joystick.h>
#include <thread>
#include <fcntl.h>
#include <signal.h>
#include <wiringPi.h>
#include <chrono>


#define PREV_STATE -1
#define NEXT_STATE 1

//Joystick buttons and sticks (axis)
#define BTN_TOGGLE_MODE 6
#define BTN_BEEP 2
#define AXIS_HORZONTAL 0
#define AXIS_VERTICAL 4
#define AXIS_SPOOL 2
#define AXIS_FIRE 5

//Pins
#define SERVO_WP_PIN 26 //GPIO 12 PWM0, PIN 32
#define DC_MOTOR_MOV_PIN 21 //GPIO 5
#define DC_MOTOR_DIR_PIN 22 //GPIO 6
#define BEEPER_PIN 8 //GPIO 2, PIN 3
#define SPOOL_PIN 4 //GPIO 23, PIN 16
#define FIRE_PIN 5 //GPIO 24, PIN 18

//Maximums and Minimums
#define MAX_JSTICK 32767
#define MAX_SERVO 250
#define MIN_SERVO 220

#define DEAD_ZONE 10

//Global
extern bool RUN_MANUAL;

//Public Functions
int RunIdle();
int RunObjDetect();
int RunTargetWarn();
int RunTargetFire();

//Constructor and destructor
void InitCL();
void DestructCL();

#endif