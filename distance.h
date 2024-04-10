/* distance.h
*/
#ifndef DISTANCE_H
#define DISTANCE_H

#include <stdint.h>

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

void InitDist(bool show_image);
void DestructDist();
int GetClosestTarget(target_info* t, float *dist);


#endif
