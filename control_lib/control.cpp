/* control.cpp
 *
*/
#include "control.h"

//Private globals
std::shared_ptr<spdlog::logger> LOGGER;
std::thread js_thread;
int JOYSTICK_FD = -1;
bool RUN_MANUAL = false;

//Important Private Functions
void JoyStickControl();


void SigHandle(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        //Exiting 
        printf("\nExiting via sig handle...\n");
        DestructCL();
        exit(1);
    }
}

void InitCL(std::shared_ptr<spdlog::logger> logger)
{
        LOGGER = logger;
        signal(SIGINT, SigHandle);
        signal(SIGTERM, SigHandle);

        //Initialize joystick
        JOYSTICK_FD = open("/dev/input/js0", O_RDONLY);
        if (JOYSTICK_FD == -1)
        {
            logger->error("Unable to open joystick...");
        }
        else
        {
            //Startup joystick controller thread
            js_thread = std::thread(JoyStickControl);
        }

    };

void DestructCL()
{
    printf("Exiting control library...\n");
    if (JOYSTICK_FD != -1)
        close(JOYSTICK_FD);
}


/* Waits until interrupted by the motion sensors.
 * Returns 0 when ready awakened.
*/
int RunIdle()
{
    LOGGER->debug("System in IDLE state.");
    int test_count = 5;
    while(!RUN_MANUAL)
    {
        sleep(1);

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
    return 0;
}


int RunObjDetect()
{
    LOGGER->debug("System in Object Detection state.");

    //Utilize cv_lib

    //Object is detected and identified as the target

    //Move system to track the target

    //Target gets too close, start up warning state
    sleep(1);
    return NEXT_STATE;
}

int RunTargetWarn()
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

int RunTargetFire()
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

/* Move toy verticalically
*/
int MoveServo(int value)
{
    LOGGER->debug("Moving servo");
    return 0;
}

/* Move toy horizontally
*/
int MoveStepper(int value)
{
    LOGGER->debug("Moving stepper towards");
    return 0;
}

void Beep(bool on)
{
    if (on)
        LOGGER->debug("Beep!");
}

void Spool(int value)
{
    LOGGER->debug("Spooling!");
}

void Fire(int value)
{
    LOGGER->debug("Fire!");
}

/* Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

/* Current state of an axis.
 */
struct axis_state 
{
    short x, y;
};

/* Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
{
    size_t axis = event->number / 2;

    if (axis < 3)
    {
        if (event->number % 2 == 0)
            axes[axis].x = event->value;
        else
            axes[axis].y = event->value;
    }

    return axis;
}

/* Manual control via a bluetooth controller.
 * '-' sign toggles between manual or AI controlling mode.
*/
void JoyStickControl()
{
    if (JOYSTICK_FD != -1)
    {
        struct js_event event;
        struct axis_state axes[3] = {0};
        size_t axis;

        // This loop will exit if the controller is unplugged.
        while (read_event(JOYSTICK_FD, &event) == 0)
        {
            if (event.type == JS_EVENT_BUTTON && event.number == BTN_TOGGLE_MODE && event.value == true)
            {
                RUN_MANUAL = !RUN_MANUAL;
                if (RUN_MANUAL)
                    LOGGER->debug("Transitioning to manual mode");
                else
                    LOGGER->debug("Transitioning to autonomous mode");
            }

            if (RUN_MANUAL)
            {
                //BUTTONS
                if (event.type == JS_EVENT_BUTTON) //Pressed button
                {
                    //printf("Button %u %s\n", event.number, event.value ? "pressed" : "released");
                    if(event.number == BTN_BEEP)
                    {
                        Beep(event.value);
                    }
                }
                //AXIS
                else if (event.type == JS_EVENT_AXIS)
                {
                    axis = get_axis_state(&event, axes);
                    printf("Axis %zu at (%6d, %6d) %u\n", axis, axes[axis].x, axes[axis].y, event.number);

                    if (event.number == AXIS_HORZONTAL) MoveStepper(axes[axis].x);
                    if (event.number == AXIS_VERTICAL)  MoveServo(axes[axis].y);
                    if (event.number == AXIS_SPOOL)     Spool(axes[axis].x);
                    if (event.number == AXIS_FIRE)      Fire(axes[axis].y);
                }
            }
        }
    }
}