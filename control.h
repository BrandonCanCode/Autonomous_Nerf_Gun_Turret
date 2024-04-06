/* control.h
 *
*/
#ifndef CONTROL_LIB_H
#define CONTROL_LIB_H

#include "distance.h"
#include "joystick.h"
#include "spdlog/spdlog.h"
#include <unistd.h>
#include <stdio.h>
#include <thread>
#include <signal.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <chrono>
#include <stdint.h>
#include <float.h>

#define PREV_STATE -1
#define NEXT_STATE 1


//Pins
#define SERVO_WP_PIN 26 //GPIO 12 PWM0, PIN 32
#define DC_MOTOR_MOV_PIN 21 //GPIO 5
#define DC_MOTOR_DIR_PIN 22 //GPIO 6
#define BEEPER_PIN 8 //GPIO 2, PIN 3
#define SPOOL_PIN 4 //GPIO 23, PIN 16
#define FIRE_PIN 5  //GPIO 24, PIN 18
#define PIR0_PIN 29 //GPIO 21, PIN 40
#define PIR1_PIN 25 //GPIO 26, PIN 37
#define PIR2_PIN 24 //GPIO 19, PIN 25
#define PIR3_PIN 23 //GPIO 13, PIN 33
#define PIR4_PIN 30 //GPIO 0,  PIN 27

//Maximums and Minimums
#define MAX_JSTICK 32767
#define MAX_SERVO 250
#define MIN_SERVO 216
#define MAX_TIMEOUT_S 4

#define RIGHT 1
#define LEFT 0
#define UP -1
#define DOWN 1
#define STOP 0
// #define Kp 0.4   //0.3
// #define Kd 0.55  //0.002


//Distances
#define WARNING_DIST 8
#define FIRE_DIST 3

//Macro functions
#define max(x,y) (x > y ? x : y)
#define min(x,y) (x < y ? x : y)

//Global
extern bool RUN_MANUAL;

//Public Functions
int RunIdle();
int RunObjDetect();
int RunTargetWarn();
int RunTargetFire();

//Constructor and destructor
void InitCL(float K_p, float K_d);
void DestructCL();

void StopEverything();
void MoveDCMotor(int value, bool DIR = 0);
void MoveServo(int value);
void Beep(bool on);
void Fire(bool on);
void Spool(bool on);
void StopFollowObjThread();

#endif