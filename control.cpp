/* control.cpp
 *
*/
#include "control.h"

//Private globals
extern std::shared_ptr<spdlog::logger> LOG;
std::thread js_thread;
std::thread servo_thread;
std::thread stepper_thread;
std::thread follow_thread;
bool RUN_MANUAL = true;
bool STOP_THREADS = false;
bool FOLLOW_OBJ = false;

//Thread variables
int JOYSTICK_FD = -1;
int SERVO_DIR = 0;

//Depth Sensor Variables
int shmid;
target_info *shared_mem;
rs2_error* e;
rs2_context* ctx;
rs2_device_list* device_list;
rs2_device* dev;
rs2_pipeline* pipeline;
rs2_config* config;
rs2_pipeline_profile* pipeline_profile;
float target_distance = -1.0;


//Thread Functions
void JoyStickControlThread();
void MoveServoThread();
void FollowObjectThread();

//Other private functions
void MoveDCMotor(int value);
void Beep(bool on);
void Fire(bool on);
void Spool(bool on);

void InitDist();
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

        //Configure Other PINs
        pinMode(SPOOL_PIN, OUTPUT);
        pinMode(FIRE_PIN, OUTPUT);
        pinMode(BEEPER_PIN, OUTPUT);
        pinMode(PIR1_PIN, INPUT);
        pinMode(PIR2_PIN, INPUT);
        pinMode(PIR3_PIN, INPUT);
        pinMode(PIR4_PIN, INPUT);

        //Initialize depth sensor
        InitDist();
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
    FOLLOW_OBJ = false;
    
    //Destroy a follow thread if running
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (follow_thread.joinable())
    {
        follow_thread.join();
    }

    while(!RUN_MANUAL)
    {
        //Check sensors
        int P1 = digitalRead(PIR1_PIN); //NE
        int P2 = digitalRead(PIR2_PIN); //SE
        int P3 = digitalRead(PIR3_PIN); //SW
        int P4 = digitalRead(PIR4_PIN); //NW
        printf("P1=%d P2=%d P3=%d P4=%d\n",P1,P2,P3,P4);
        
        //North
        if (P1 && P4)
        {
            LOG->debug("Motion detected North (PIR1 and PIR2)");
        }
        //South
        else if (P2 && P3)
        {
            LOG->debug("Motion detected South (PIR2 and PIR3)");
            MoveDCMotor(RIGHT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*5));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        //East
        else if (P1 && P2)
        {
            LOG->debug("Motion detected East (PIR1 and PIR2)");
            MoveDCMotor(RIGHT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*3));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        //West
        else if (P3 && P4)
        {
            LOG->debug("Motion detected West (PIR3 and PIR4)");
            MoveDCMotor(LEFT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*3-58));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        //NE
        else if (P1)
        {
            LOG->debug("Motion detected North East (PIR1)");
            MoveDCMotor(RIGHT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*2));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        //NW
        else if (P4)
        {
            LOG->debug("Motion detected North West (PIR4)");
            MoveDCMotor(LEFT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*2));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        //SE
        else if (P2)
        {
            LOG->debug("Motion detected South East (PIR2)");
            MoveDCMotor(RIGHT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*4));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        //SW
        else if (P3)
        {
            LOG->debug("Motion detected South West (PIR3)");
            MoveDCMotor(LEFT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*4+40));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        else
        {
            LOG->debug("No motion detected. Idle-ing...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    return 0;
}


int RunObjDetect()
{
    LOG->debug("System in Object Detection state.");

    // Start up object following thread
    FOLLOW_OBJ = true;
    follow_thread = std::thread(FollowObjectThread);

    // Run until we think the target is in warning radius
    char message[256];
    int time_out = MAX_TIMEOUT_S;
    while (0 < time_out)
    {
        if (target_distance == -1.0)
        {
            time_out--;
        }
        else
        {
            //Reset timeout
            sprintf(message, "Target is %.f meters away", target_distance);
            LOG->debug(message);
            time_out = MAX_TIMEOUT_S;

            //Check if we need to go to warning
            if (target_distance <= WARNING_DIST)
                return NEXT_STATE;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return PREV_STATE;
}

int RunTargetWarn()
{
    LOG->debug("System in Target Warning state.");

    // Run until we think the target is in fire radius or timeout
    int time_out = MAX_TIMEOUT_S;
    int beep_count = 0;
    while (0 < time_out)
    {
        if (target_distance == -1.0)
        {
            time_out--;
        }
        else
        {
            //Reset timeout
            time_out = MAX_TIMEOUT_S;

            //Check if we need to go to warning
            if (target_distance <= FIRE_DIST)
            {
                Beep(false);
                return NEXT_STATE;
            }
        }
        
        //Run beeper every 4 seconds
        beep_count++;
        if (beep_count == 4) 
        {
            Beep(true);
            beep_count = 0;
        }
        else
            Beep(false);
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return PREV_STATE;
}


int RunTargetFire()
{
    LOG->debug("System in Target Fire state.");

    //Sound fire
    Spool(true);
    Beep(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Beep(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Beep(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Beep(false);

    //Spool
    std::this_thread::sleep_for(std::chrono::seconds(1));

    //Fire a burst shot
    Fire(true);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    Fire(false);
    Spool(false);

    //Give a 5 second cool down
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return PREV_STATE;
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
    else if (value < -DEAD_ZONE) //Move down
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
    else if (value < -DEAD_ZONE) //Move left
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
    {
        LOG->debug("Fire!");
        digitalWrite(FIRE_PIN, 1);
    }
    else
        digitalWrite(FIRE_PIN, 0);
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




/* Function calls to librealsense may raise errors of type rs_error*/
void check_error(rs2_error* e)
{
    if (e)
    {
        printf("rs_error was raised when calling %s(%s):\n", rs2_get_failed_function(e), rs2_get_failed_args(e));
        printf("    %s\n", rs2_get_error_message(e));
        exit(EXIT_FAILURE);
    }
}

void print_device_info(rs2_device* dev)
{
    rs2_error* e = 0;
    printf("\nUsing device 0, an %s\n", rs2_get_device_info(dev, RS2_CAMERA_INFO_NAME, &e));
    check_error(e);
    printf("    Serial number: %s\n", rs2_get_device_info(dev, RS2_CAMERA_INFO_SERIAL_NUMBER, &e));
    check_error(e);
    printf("    Firmware version: %s\n\n", rs2_get_device_info(dev, RS2_CAMERA_INFO_FIRMWARE_VERSION, &e));
    check_error(e);
}


void InitDist()
{
    e = 0;

    // Create a context object. This object owns the handles to all connected realsense devices.
    // The returned object should be released with rs2_delete_context(...)
    ctx = rs2_create_context(RS2_API_VERSION, &e);
    check_error(e);

    /* Get a list of all the connected devices. */
    // The returned object should be released with rs2_delete_device_list(...)
    device_list = rs2_query_devices(ctx, &e);
    check_error(e);

    int dev_count = rs2_get_device_count(device_list, &e);
    check_error(e);
    printf("There are %d connected RealSense devices.\n", dev_count);
    if (0 == dev_count)
        return;

    // Get the first connected device
    // The returned object should be released with rs2_delete_device(...)
    dev = rs2_create_device(device_list, 0, &e);
    check_error(e);

    print_device_info(dev);

    // Create a pipeline to configure, start and stop camera streaming
    // The returned object should be released with rs2_delete_pipeline(...)
    pipeline = rs2_create_pipeline(ctx, &e);
    check_error(e);

    // Create a config instance, used to specify hardware configuration
    // The retunred object should be released with rs2_delete_config(...)
    config = rs2_create_config(&e);
    check_error(e);

    // Request a specific configuration
    rs2_config_enable_stream(config, RS2_STREAM_DEPTH, 0, WIDTH, HEIGHT, RS2_FORMAT_Z16, 30, &e);
    check_error(e);

    // Start the pipeline streaming
    // The retunred object should be released with rs2_delete_pipeline_profile(...)
    pipeline_profile = rs2_pipeline_start_with_config(pipeline, config, &e);
    if (e)
    {
        LOG->error("The connected device doesn't support depth streaming!");
        exit(EXIT_FAILURE);
    }

    //Setup shared memory
    key_t key = ftok("/tmp", 42); // Generate a key for the shared memory segment
    shmid = shmget(key, MAX_TARGETS*sizeof(target_info), IPC_CREAT | 0666);
    shared_mem = (target_info *)shmat(shmid, NULL, 0);

    // Initialize target_info structs
    for (int i=0; i<MAX_TARGETS; i++) 
    {
        shared_mem[i].target = i; // Example: set target value
        shared_mem[i].x = 0;      // Initialize x coordinate
        shared_mem[i].y = 0;      // Initialize y coordinate
    }

    //Start John's Object Detection (Python script)
    system("nohup python3 set_target.py 0 0 0 &");
}


void DestructDist()
{
    // Stop the pipeline streaming
    rs2_pipeline_stop(pipeline, &e);
    check_error(e);

    // Release resources
    rs2_delete_pipeline_profile(pipeline_profile);
    rs2_delete_config(config);
    rs2_delete_pipeline(pipeline);
    rs2_delete_device(dev);
    rs2_delete_device_list(device_list);
    rs2_delete_context(ctx);

    //Stop John's Object Detection (python script)
    system("pkill -f set_target.py");
    
    // Detach and destroy the shared memory segment
    shmdt(shared_mem); 
    shmctl(shmid, IPC_RMID, NULL);
}


void GetDistances(float *distances)
{
    //Remove rest of distnaces
    for(int i=0; i<MAX_TARGETS; i++)
    {
        distances[i] = -1.0;
    }

    // This call waits until a new composite_frame is available
    // composite_frame holds a set of frames. It is used to prevent frame drops
    // The returned object should be released with rs2_release_frame(...)
    rs2_frame* frames = rs2_pipeline_wait_for_frames(pipeline, RS2_DEFAULT_TIMEOUT, &e);
    check_error(e);

    // Returns the number of frames embedded within the composite frame
    int num_of_frames = rs2_embedded_frames_count(frames, &e);
    check_error(e);

    for (int i=0; i<num_of_frames; i++)
    {
        // The retunred object should be released with rs2_release_frame(...)
        rs2_frame* frame = rs2_extract_frame(frames, i, &e);
        check_error(e);

        // Check if the given frame can be extended to depth frame interface
        // Accept only depth frames and skip other frames
        if (0 == rs2_is_frame_extendable_to(frame, RS2_EXTENSION_DEPTH_FRAME, &e))
            continue;

        // Get the depth frame's dimensions
        // int width = rs2_get_frame_width(frame, &e);
        // check_error(e);
        // int height = rs2_get_frame_height(frame, &e);
        // check_error(e);

        // Query the distance from the camera to the object in the center of the image
        // float dist_to_center = rs2_depth_frame_get_distance(frame, width / 2, height / 2, &e);
        // check_error(e);

        // Set the distances
        for(int j=0; j<MAX_TARGETS; j++)
        {
            if (shared_mem[j].x != 0 && shared_mem[j].y != 0)
            {
                distances[j] = rs2_depth_frame_get_distance(frame, shared_mem[j].x, shared_mem[j].y, &e);
                check_error(e);
                printf("Target %d (%d,%d) is %.3f meters away.\n", 
                    shared_mem[j].target, shared_mem[j].x, shared_mem[j].y, distances[j]);
            }
        }

        rs2_release_frame(frame);
    }

    rs2_release_frame(frames);
}


int GetClosestTarget(target_info* t)
{
    float close_distance = FLT_MAX;
    int index = -1;
    float distances[MAX_TARGETS];
    t = NULL;

    GetDistances(distances);
    for(int i=0; i<MAX_TARGETS; i++)
    {
        if (distances[i] != -1 && distances[i] < close_distance)
        {
            close_distance = distances[i];
            index = i;
        }
    }

    if (index == -1) 
    {
        target_distance = -1.0;
        return 1;
    }
    else
    {
        t = &shared_mem[index];
        target_distance = close_distance;
    }

    return 0;
}

/* Follow the closest target
*/
void FollowObjectThread()
{
    LOG->debug("Starting follow thread!");

    target_info *t = NULL;
    while(!STOP_THREADS && FOLLOW_OBJ)
    {
        if(!GetClosestTarget(t))
        {
            //Move left
            if (t->x < CENTER_X-PIXEL_RADIUS)  
                MoveDCMotor(LEFT);  
            //Move right
            else if (CENTER_X+PIXEL_RADIUS < t->y) 
                 MoveDCMotor(RIGHT);  
            //STOP
            else                                    
                MoveDCMotor(STOP);

            //Move UP
            if (t->y < CENTER_Y-PIXEL_RADIUS)
                MoveServo(UP);
            //Move Down
            else if (CENTER_Y+PIXEL_RADIUS < t->y)
                MoveDCMotor(DOWN);
            //STOP
            else
                MoveDCMotor(STOP);
        }
    }

    LOG->debug("Exiting follow thread!");
}