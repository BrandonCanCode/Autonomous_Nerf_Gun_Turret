/* control.h
 *
*/
#ifndef CONTROL_LIB_H
#define CONTROL_LIB_H

#include "spdlog/spdlog.h"
#include "../cv_lib/computer_vision.h"
#include <unistd.h>
#include <stdio.h>


#define PREV_STATE -1
#define NEXT_STATE 1


class control_lib {
private:
    std::shared_ptr<spdlog::logger> LOGGER;

public:
    //Constructor
    control_lib(std::shared_ptr<spdlog::logger> logger)
    {
        LOGGER = logger;
    }
    
    int RunIdle();
    int RunObjDetect();
    int RunTargetWarn();
    int RunTargetFire();
    int move_stepper(int dir);
    int move_servo(int dir);
};

#endif