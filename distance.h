#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <thread>
#include "spdlog/spdlog.h"

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

void InitDist();
void DestructDist();
void RunDistanceThread();