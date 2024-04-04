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
#include <softPwm.h>
#include <chrono>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <float.h>

/* Include the librealsense C header files */
#include <librealsense2/rs.h>
#include <librealsense2/h/rs_pipeline.h>
#include <librealsense2/h/rs_option.h>
#include <librealsense2/h/rs_frame.h>

#define MAX_TARGETS 5

typedef struct {
    uint32_t target;
    uint32_t x;
    uint32_t y;
} target_info;


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
#define FIRE_PIN 5  //GPIO 24, PIN 18
#define PIR1_PIN 25 //GPIO 26, PIN 37
#define PIR2_PIN 24 //GPIO 19, PIN 25
#define PIR3_PIN 23 //GPIO 13, PIN 33
#define PIR4_PIN 30 //GPIO 0,  PIN 27

//Maximums and Minimums
#define DEAD_ZONE 10
#define MAX_JSTICK 32767
#define MAX_SERVO 250
#define MIN_SERVO 216
#define MAX_TIMEOUT_S 10

#define RIGHT MAX_JSTICK
#define LEFT -1*MAX_JSTICK
#define UP DEAD_ZONE+1
#define DOWN -DEAD_ZONE-1
#define STOP 0
#define MILISECONDS_PIR_STEP 250 //1/10

//Camera stuff
#define WIDTH 640
#define HEIGHT 480
#define CENTER_X WIDTH/2
#define CENTER_Y HEIGHT/2
#define PIXEL_RADIUS 40

//Distances
#define WARNING_DIST 7
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
void InitCL();
void DestructCL();

#endif