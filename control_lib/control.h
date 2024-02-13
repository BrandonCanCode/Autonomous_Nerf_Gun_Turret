/* control.h
 *
*/
#ifndef CONTROL_LIB_H
#define CONTROL_LIB_H

#include "spdlog/spdlog.h"
#include "../cv_lib/computer_vision.h"
#include <unistd.h>

#define PREV_STATE 1
#define NEXT_STATE 2


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

};

#endif