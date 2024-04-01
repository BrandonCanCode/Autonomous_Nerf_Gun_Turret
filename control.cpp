/* control.cpp
 *
*/
#include "control.h"

//Private globals
extern std::shared_ptr<spdlog::logger> LOG;
std::thread js_thread;
std::thread servo_thread;
std::thread stepper_thread;
bool RUN_MANUAL = true;
bool STOP_THREADS = false;

//Thread variables
int JOYSTICK_FD = -1;
int SERVO_DIR = 0;

//Thread Functions
void JoyStickControlThread();
void MoveServoThread();
void MoveDCMotor(int value);

//Other private functions
void Beep(bool on);
void Fire(bool on);
void Spool(bool on);
void DestructDist();

void InitCL()
{
        //Initialize joystick thread
        js_thread = std::thread(JoyStickControlThread);

        //Initialize wiringPi
        if (wiringPiSetup() == -1)
        {
            printf ("Setup wiringPi Failed!\n");
            DestructCL();
            exit(1);
        }
        printf ("Reminder: this program must be run with sudo.\n");

        //Configure Servo PIN
        pinMode (SERVO_WP_PIN, PWM_OUTPUT);
        pwmSetMode(PWM_MODE_MS);
        pwmSetClock (1651);  //10ms period
        pwmSetRange(1000);

        //Start servo thread
        servo_thread = std::thread(MoveServoThread);

        //Configure DC motor PINs
        pinMode(DC_MOTOR_MOV_PIN, OUTPUT);
        pinMode(DC_MOTOR_DIR_PIN, OUTPUT);

        //Start stepper thread
        //stepper_thread = std::thread(MoveDCMotorThread);

        //Configure Other PINs
        pinMode(SPOOL_PIN, OUTPUT);
        pinMode(FIRE_PIN, OUTPUT);
        pinMode(BEEPER_PIN, OUTPUT);

}

void DestructCL()
{
    //Stop motors
    SERVO_DIR = 0;

    //Stop other functions
    Beep(false);
    Fire(false);
    Spool(false);
    MoveDCMotor(0);

    STOP_THREADS = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    DestructDist();
    printf("Exiting control library...\n");
}


/* Waits until interrupted by the motion sensors.
 * Returns 0 when ready awakened.
*/
int RunIdle()
{
    LOG->debug("System in IDLE state.");
    bool awaken = false;

    while(!RUN_MANUAL)
    {
        //Check sensors
        if (awaken)
        {
            LOG->debug("Motion detected!");
            return NEXT_STATE;
        }
        else
        {
            LOG->debug("No motion detected. Idle-ing...");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    return 0;
}


int RunObjDetect()
{
    LOG->debug("System in Object Detection state.");

    //Utilize cv_lib

    //Object is detected and identified as the target

    //Move system to track the target

    //Target gets too close, start up warning state
    sleep(1);
    return NEXT_STATE;
}

int RunTargetWarn()
{
    LOG->debug("System in Target Warning state.");

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
    LOG->debug("System in Target Fire state.");

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
 * Negative value = up
 * Positive value = down
*/
void MoveServo(int value)
{
    //Translate joystick value to [up 1, stop 0, down -1]
    if (-DEAD_ZONE <= value && value <= DEAD_ZONE) //stop
    {
        SERVO_DIR = 0;
    }
    else if (value < DEAD_ZONE) //Move down
    {
        SERVO_DIR = 1;
    }
    else if (value > DEAD_ZONE) //Move up
    {
        SERVO_DIR = -1;
    }
}

void MoveServoThread()
{
    int position = 230;

    while(!STOP_THREADS)
    {
        if (SERVO_DIR == 0) //stop
        {
            position = position;
        }
        else if (SERVO_DIR > 0) //Move up
        {
            if (position > MIN_SERVO)
                position--;
        }
        else if (SERVO_DIR < 0) //Move down
        {
            if (position < MAX_SERVO)
                position++;
        }
        pwmWrite(SERVO_WP_PIN, position);

        if (SERVO_DIR > 0) //UP
            std::this_thread::sleep_for(std::chrono::milliseconds(45));
        else if (SERVO_DIR < 0) //DOWN
            std::this_thread::sleep_for(std::chrono::milliseconds(55));
    }
}

/* Move toy horizontally
*/
void MoveDCMotor(int value)
{
    //Translate joystick value to [right 1, stop 0, left -1]
    if (-DEAD_ZONE <= value && value <= DEAD_ZONE) //stop
    {
        LOG->debug("Stopping DC motor");
        digitalWrite(DC_MOTOR_MOV_PIN, 0);
        digitalWrite(DC_MOTOR_DIR_PIN, 0);
    }
    else if (value < DEAD_ZONE) //Move left
    {
        LOG->debug("Moving DC motor left");
        digitalWrite(DC_MOTOR_MOV_PIN, 0);
        digitalWrite(DC_MOTOR_DIR_PIN, 1);
    }
    else if (value > DEAD_ZONE) //Move right
    {
        LOG->debug("Moving DC motor right");
        digitalWrite(DC_MOTOR_MOV_PIN, 1);
        digitalWrite(DC_MOTOR_DIR_PIN, 0);
    }
}

void Beep(bool on)
{
    if (on)
    {
        LOG->debug("Beep!");
        digitalWrite(BEEPER_PIN, 1);
    }
    else
        digitalWrite(BEEPER_PIN, 0);
}

void Spool(bool on)
{
    if (on)
    {
        LOG->debug("Spooling!");
        digitalWrite(SPOOL_PIN, 1);
    }
    else
        digitalWrite(SPOOL_PIN, 0);
    
    
}

void Fire(bool on)
{
    if (on)
        LOG->debug("Fire!");

    digitalWrite(FIRE_PIN, (int)on);
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
void JoyStickControlThread()
{
    const char *device = "/dev/input/js0";
    struct js_event event;
    struct axis_state axes[3] = {0};
    size_t axis;
    RUN_MANUAL = false;

    while (!STOP_THREADS)
    {
        // Open joystick device
        while (!STOP_THREADS && JOYSTICK_FD == -1)
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
                }
                
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        // Joystick is connected and taking inputs
        // This loop will exit if the controller is unplugged.
        while (!STOP_THREADS && read_event(JOYSTICK_FD, &event) == 0)
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

                    if (event.number == AXIS_HORZONTAL) MoveDCMotor(axes[axis].x);
                    if (event.number == AXIS_VERTICAL)  MoveServo(axes[axis].x);
                    if (event.number == AXIS_SPOOL)     Spool(axes[axis].x > 0 ? true : false);
                    if (event.number == AXIS_FIRE)      Fire(axes[axis].y > 0 ? true : false);
                }
            }
        }

        //When reading ends, close device so we can reconnect later
        close(JOYSTICK_FD);
        JOYSTICK_FD = -1;
    }
}