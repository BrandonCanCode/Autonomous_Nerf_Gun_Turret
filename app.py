# app.py
# Main loop that runs on the Raspberry Pi 5.
# 

from logger import setup_logger
from control import *


# States
IDLE=0
OBJ_DETECT=1
TARGET_WARNING=2
TARGET_FIRE=3

# Initialization
logger = setup_logger()
logger.debug("System initializing...")


# Start (main)
state = IDLE
loop = True
result = 0

try:
    while (loop):
        # Run control function for the state
        if (state == IDLE):
            state += RunIdle()
        
        elif (state == OBJ_DETECT):
            state += RunObjDetect()
        
        elif (state == TARGET_WARNING):
            state += RunTargetWarn()

        elif (state == TARGET_FIRE):
            state += RunTargetFire()
                
        else:  #Should never occur
            logger.error("Invalid state!")
            loop = False
        
        # Fix state if set to invalid index
        if (state < 0):
            state = IDLE
        elif (state > 3):
            state = TARGET_FIRE
            
except Exception as e:
    logger.critical(f"Exception occurred: {e}")


logger.debug("System closing...")