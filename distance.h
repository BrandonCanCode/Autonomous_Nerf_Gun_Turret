/* distance.h
*/
#ifndef DISTANCE_H
#define DISTANCE_H

#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "spdlog/spdlog.h"

/* Include the librealsense C header files */
#include <librealsense2/rs.h>
#include <librealsense2/h/rs_pipeline.h>
#include <librealsense2/h/rs_option.h>
#include <librealsense2/h/rs_frame.h>

#define MAX_TARGETS 5

//Camera stuff
#define WIDTH 640
#define HEIGHT 480
#define CENTER_X WIDTH/2
#define CENTER_Y HEIGHT/2
#define PIXEL_RADIUS 40

typedef struct {
    uint32_t target;
    uint32_t x;
    uint32_t y;
} target_info;

//Shared globals
extern float TARGET_DIST;

void InitDist();
void DestructDist();
int GetClosestTarget(target_info* t);


#endif