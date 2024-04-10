#include "joystick.h"
#include "control.h"
#include <spdlog/spdlog.h>
#include <linux/joystick.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>

//Shared Globals
extern std::shared_ptr<spdlog::logger> LOG;

//Globals
bool STOP_JS_THREAD = false;
bool STOP_SERVO_THREAD = false;
int JOYSTICK_FD = -1;
std::thread js_thread;
std::thread servo_thread;
char mssg[256];

struct axis_state {
    short x, y;
};

//Thread variables
int SERVO_DIR = 0;

//Functions
void JoyStickControlThread();
void MoveServoThread();


void InitJS()
{
    LOG->debug("Initializing joystick library");

    //Initialize joystick thread
    js_thread = std::thread(JoyStickControlThread);
    servo_thread = std::thread(MoveServoThread);
}

void DestructJS()
{
    //Stop the joystick thread
    SERVO_DIR = 0;
    STOP_JS_THREAD = true;
    STOP_SERVO_THREAD = true;
    LOG->debug("Exiting joystick library...");
}



/* Reads a joystick event from the joystick device.
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


/* Translate joystick value to DC motor value
*/
void MoveDCMotorForJS(int value)
{
    if (-DEAD_ZONE <= value && value <= DEAD_ZONE) //stop
    {
        MoveDCMotor(0, 0);
    }
    else if (value < -DEAD_ZONE) //Move left
    {
        MoveDCMotor(-1.0*(value/(float)MAX_JSTICK)*100.0, 0);
    }
    else if (value > DEAD_ZONE) //Move right
    {
        MoveDCMotor((value/(float)MAX_JSTICK)*100.0, 1);
    }
}


/* Move toy verticalically
 * Negative value = up
 * Positive value = down
*/
void MoveServoForJS(int value)
{
    //Translate joystick value
    if (-DEAD_ZONE <= value && value <= DEAD_ZONE) //stop
    {
        SERVO_DIR = STOP;
    }
    else if (value < -DEAD_ZONE)
    {
        SERVO_DIR = UP;
    }
    else if (value > +DEAD_ZONE)
    {
        SERVO_DIR = DOWN;
    }
}


void MoveServoThread()
{
    LOG->debug("Starting Servo Thread");

    while(!STOP_SERVO_THREAD)
    {
        if (SERVO_DIR == UP) //Move up
        {
            printf("UP\n");
            MoveServo(UP);
            std::this_thread::sleep_for(std::chrono::milliseconds(45));
        }
        else if (SERVO_DIR == DOWN) //Move down
        {
            printf("DOWN\n");
            MoveServo(DOWN);
            std::this_thread::sleep_for(std::chrono::milliseconds(55));
        }
    }
    LOG->debug("Exiting Servo Thread");
}


/* Manual control via a bluetooth controller.
 * '-' sign toggles between manual or AI controlling mode.
*/
void JoyStickControlThread()
{
    const char *device = "/dev/input/js0";
    struct js_event event;
    struct axis_state axes[3] = {0};
    size_t axis;
    RUN_MANUAL = true;

    try
        {
        while (!STOP_JS_THREAD)
        {
            // Open joystick device
            while (!STOP_JS_THREAD && JOYSTICK_FD == -1)
            {
                // Periodically check if joystick exists
                if (access(device, F_OK) != -1) 
                {
                    LOG->debug("Joystick discovered");

                    JOYSTICK_FD = open(device, O_RDONLY);
                    if (JOYSTICK_FD == -1)
                    {
                        LOG->error("Unable to open joystick...");
                    }
                    else
                    {
                        //Device is opened and connected, change to manual mode
                        RUN_MANUAL = true;
                        printf("\n");
                        printf("[=================]  Joystick Connected!  [=================]\n");
                        printf("|                                                           |\n");
                        printf("|           Changed turret control to manual mode           |\n");
                        printf("|  Press '-' to change between modes (autonomous or manual) |\n");
                        printf("|                                                           |\n");
                        printf("[===========================================================]\n\n");

                        //Stop anything that motors that could be running
                        StopEverything();
                        LOG->debug(">>> Changed Mode to Manual");
                    }
                    
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }

            // Joystick is connected and taking inputs
            // This loop will exit if the controller is unplugged.
            while (!STOP_JS_THREAD && read_event(JOYSTICK_FD, &event) == 0)
            {
                if (event.type == JS_EVENT_BUTTON && event.number == BTN_TOGGLE_MODE && event.value == true)
                {
                    RUN_MANUAL = !RUN_MANUAL;
                    if (RUN_MANUAL)
                        LOG->debug(">>> Changed Mode to Manual");
                    else
                        LOG->debug(">>> Changed Mode to Autonomous");
                }

                if (RUN_MANUAL)
                {
                    //Destroy the follow thread if running
                    StopFollowObjThread();

                    //BUTTONS
                    if (event.type == JS_EVENT_BUTTON) //Pressed button
                    {
                        printf("Button %u %s\n", event.number, event.value ? "pressed" : "released");
                        if(event.number == BTN_BEEP)
                        {
                            Beep(event.value);
                        }
                        if (event.number == BTN_LAZER)
                        {
                            LAZER(event.value);
                        }
                    }
                    //AXIS
                    else if (event.type == JS_EVENT_AXIS)
                    {
                        axis = get_axis_state(&event, axes);
                        //printf("Axis %zu at (%6d, %6d) %u\n", axis, axes[axis].x, axes[axis].y, event.number);

                        if (event.number == AXIS_HORIZONTAL) MoveDCMotorForJS(axes[axis].x);
                        if (event.number == AXIS_VERTICAL)   MoveServoForJS(axes[axis].x);
                        if (event.number == AXIS_SPOOL)      Spool(axes[axis].x > 0 ? true : false);
                        if (event.number == AXIS_FIRE)       Fire(axes[axis].y > 0 ? true : false);
                    }
                }
            }

            //When reading ends, close device so we can reconnect later
            close(JOYSTICK_FD);
            JOYSTICK_FD = -1;
        }
    }
    catch(const std::exception& e)
    {
        sprintf(mssg, e.what());
        LOG->error(mssg);
    }
}
