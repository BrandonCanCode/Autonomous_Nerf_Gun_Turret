/* control.cpp
 *
*/

#include "control.h"

/* Waits until interrupted by the motion sensors.
 * Returns 0 when ready awakened.
*/
int control_lib::RunIdle()
{
    LOGGER->debug("System in IDLE state.");
    int test_count = 5;
    while(1)
    {
        sleep(2);

        //Check sensors
        //Pretend sensors are awakened
        if (test_count--)
        {
            LOGGER->debug("No motion detected. Idle-ing...");
        }
        else
        {
            return NEXT_STATE;
        }
    }
}


int control_lib::RunObjDetect()
{
    LOGGER->debug("System in Object Detection state.");

    //Utilize cv_lib

    //Object is detected and identified as the target

    //Move system to track the target

    //Target gets too close, start up warning state
    sleep(1);
    return NEXT_STATE;
}

int control_lib::RunTargetWarn()
{
    LOGGER->debug("System in Target Warning state.");

    //Keep track of target
    //Move system to track the target

    //Run beeps to warn target they are getting to close

    //Target left warning distance go to prev state
    //return prev_state

    //Target is now in firing distance, go to next state
    sleep(1);
    return NEXT_STATE;
}

int control_lib::RunTargetFire()
{
    LOGGER->debug("System in Target Fire state.");

    //Keep track of target
    //Move system to track the target

    //Send out beep
    //Shoot 1 burst at target
    //Wait for target to leave (5 seconds?)

    //Target left danger zone? go to prev state
    sleep(1);
    return PREV_STATE;

    //else shoot after waiting
}