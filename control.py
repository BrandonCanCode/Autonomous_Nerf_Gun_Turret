# control.cpp
#

from time import sleep
from gpiozero import MotionSensor, Servo
from logger import setup_logger

logger = setup_logger()
PREV_STATE = -1
NEXT_STATE = 1

# Configure motion sensors
pir_1 = MotionSensor(17)
pir_2 = MotionSensor(27)
pir_3 = MotionSensor(22)
pir_4 = MotionSensor(23)
pir_5 = MotionSensor(24)
pirs = [pir_1, pir_2, pir_3, pir_4, pir_5]

# Waits until interrupted by the motion sensors.
# Returns 0 when ready awakened.
def RunIdle():
    logger.debug("System in IDLE state.")
    
   # Continuously check each motion sensor for movement
    while(1):
        for i, pir in enumerate(pirs, start=1):
            # Motion detected
            if (pir.motion_detected):
                logger.debug(f"Motion detected at PIR {i}")
                #Move lazy susan to motion detector location
                return NEXT_STATE
        sleep(1)
        
        
    


def RunObjDetect():
    logger.debug("System in Object Detection state.")
    sleep(1)
    
    #Utilize cv_lib

    #Object is detected and identified as the target

    #Move system to track the target

    #Target gets too close, start up warning state
    return NEXT_STATE


def RunTargetWarn():
    logger.debug("System in Target Warning state.")
    sleep(1)

    #Keep track of target
    #Move system to track the target

    #Run beeps to warn target they are getting to close

    #Target left warning distance go to prev state
    #return prev_state

    #Target is now in firing distance, go to next state
    return NEXT_STATE


def RunTargetFire():
    logger.debug("System in Target Fire state.")
    sleep(1)

    #Keep track of target
    #Move system to track the target

    #Send out beep
    #Shoot 1 burst at target
    #Wait for target to leave (5 seconds?)

    #Target left danger zone? go to prev state
    return PREV_STATE

    #else shoot after waiting
