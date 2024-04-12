/* control.cpp
 *
*/
#include "control.h"
#include "distance.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <wiringPi.h>
#include <softPwm.h>
#include <unistd.h>
#include <stdio.h>
#include <float.h>
#include <chrono>

extern std::shared_ptr<spdlog::logger> LOG;

//Private globals
float TARGET_DIST = -1.0;
std::thread follow_thread;
bool RUN_MANUAL = true;
bool STOP_FOLLOW_OBJ = true;
char message[256];
float Kp;
float Kd;
bool NO_FIRE = false;

//Thread Functions
void FollowObjectThread();


void InitCL(float K_p, float K_d, bool no_fire, bool show_image)
{
        //Initialize wiringPi
        if (wiringPiSetup() == -1)
        {
            LOG->error("Setup wiringPi Failed!\n");
            DestructCL();
            exit(1);
        }

        //Configure Servo PIN
        pinMode(SERVO_WP_PIN, PWM_OUTPUT);
        pwmSetMode(PWM_MODE_MS);
        pwmSetClock (1651);  //10ms period
        pwmSetRange(1000);

        //Configure DC motor PINs
        softPwmCreate(DC_MOTOR_MOV_PIN, 0, 120);
        softPwmCreate(DC_MOTOR_DIR_PIN, 0, 120);

        //Configure Other PINs
        pinMode(SPOOL_PIN, OUTPUT);
        pinMode(FIRE_PIN, OUTPUT);
        pinMode(BEEPER_PIN, OUTPUT);
        pinMode(LAZER_PIN, OUTPUT);
        pinMode(PIR0_PIN, INPUT);
        pinMode(PIR1_PIN, INPUT);
        pinMode(PIR2_PIN, INPUT);
        pinMode(PIR3_PIN, INPUT);
        pinMode(PIR4_PIN, INPUT);

        //Set gains
        Kp = K_p;
        Kd = K_d;
        sprintf(message, "Kp=%f\tKd=%f", Kp,Kd);
        LOG->debug(message);

        NO_FIRE = no_fire;

        MoveServo(UP);

        //Initialize other libraries
        InitDist(show_image);
        InitJS();
}

void DestructCL()
{
    LOG->debug("Destroying control library!");

    StopEverything();
    STOP_FOLLOW_OBJ = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    softPwmStop(DC_MOTOR_MOV_PIN);
    softPwmStop(DC_MOTOR_DIR_PIN);
    
    DestructDist();
    DestructJS();

    LOG->debug("Exiting control library...");
}

void StopEverything()
{
    Beep(false);
    Fire(false);
    Spool(false);
    LAZER(false);
    MoveDCMotor(STOP);
    MoveServo(STOP);
}


/* Waits until interrupted by the motion sensors.
 * Returns 0 when ready awakened.
*/
int RunIdle()
{
    const int speed = 60;
    const int MILISECONDS_PIR_STEP = 500; //1/10

    LOG->debug("System in IDLE state.");

    //return NEXT_STATE;

    //Destroy a follow thread if running
    StopFollowObjThread();

    while(!RUN_MANUAL)
    {
        //Check sensors
        int P0 = digitalRead(PIR0_PIN); //N
        int P1 = digitalRead(PIR1_PIN); //NE
        int P2 = digitalRead(PIR2_PIN); //SE
        int P3 = digitalRead(PIR3_PIN); //SW
        int P4 = digitalRead(PIR4_PIN); //NW
        sprintf(message, "P0=%d P1=%d P2=%d P3=%d P4=%d",P0,P1,P2,P3,P4);
        LOG->debug(message);
        
        //North
        if (P0)
        {
            LOG->debug("Motion detected North (PIR0)");
            return NEXT_STATE;
        }
        //South
        // else if (P2 && P3)
        // {
        //     LOG->debug("Motion detected South (PIR2 and PIR3)");
        //     MoveDCMotor(speed, RIGHT);
        //     std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*5));
        //     MoveDCMotor(STOP);

        //     return NEXT_STATE;
        // }
        // //East
        // else if (P1 && P2)
        // {
        //     LOG->debug("Motion detected East (PIR1 and PIR2)");
        //     MoveDCMotor(speed, RIGHT);
        //     std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*3));
        //     MoveDCMotor(STOP);

        //     return NEXT_STATE;
        // }
        // //West
        // else if (P3 && P4)
        // {
        //     LOG->debug("Motion detected West (PIR3 and PIR4)");
        //     MoveDCMotor(speed, LEFT);
        //     std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*3));
        //     MoveDCMotor(STOP);

        //     return NEXT_STATE;
        // }
        //NE
        else if (P1)
        {
            LOG->debug("Motion detected North East (PIR1)");
            MoveDCMotor(speed, RIGHT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*2));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        //NW
        else if (P4)
        {
            LOG->debug("Motion detected North West (PIR4)");
            MoveDCMotor(speed, LEFT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*2));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        //SE
        else if (P2)
        {
            LOG->debug("Motion detected South East (PIR2)");
            MoveDCMotor(speed, RIGHT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*4));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        //SW
        else if (P3)
        {
            LOG->debug("Motion detected South West (PIR3)");
            MoveDCMotor(speed, LEFT);
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISECONDS_PIR_STEP*4));
            MoveDCMotor(STOP);

            return NEXT_STATE;
        }
        else
        {
            //LOG->debug("No motion detected. Idle-ing...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    return 0;
}


int RunObjDetect()
{
    LOG->debug("System in Object Detection state.");

    if (!RUN_MANUAL)
    {
        // Start up object following thread
        if (STOP_FOLLOW_OBJ == true)
        {
            STOP_FOLLOW_OBJ = false;
            follow_thread = std::thread(FollowObjectThread);
        }

        // Run until we think the target is in warning radius
        int time_out = MAX_TIMEOUT_S;
        while (0 < time_out && !RUN_MANUAL)
        {
            if (TARGET_DIST == -1.0)
            {
                sprintf(message, "Timeout=%d (Object Detect)", time_out);
                LOG->debug(message);
                time_out--;
            }
            else
            {
                //Reset timeout
                time_out = MAX_TIMEOUT_S;

                //Check if we need to go to warning
                if (TARGET_DIST <= WARNING_DIST)
                {
                    return NEXT_STATE;
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return PREV_STATE;
}

int RunTargetWarn()
{
    LOG->debug("System in Target Warning state.");
    int beep_count = 1;

    // Run until we think the target is in fire radius or timeout
    while (!RUN_MANUAL && TARGET_DIST != -1.0 && TARGET_DIST < WARNING_DIST)
    {
        //Check if we need to go to firing state
        if (TARGET_DIST <= FIRE_DIST)
        {
            return NEXT_STATE;
        }
        
        //Run beeper every 2 seconds
        beep_count--;
        if (beep_count <= 0) 
        {
            Beep(true);
            beep_count = 2;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            Beep(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(900));
        }
        else
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return PREV_STATE;
}


int RunTargetFire()
{
    LOG->debug("System in Target Fire state.");

    if (!RUN_MANUAL)
    {
        //Sound fire
        Spool(true);
        Beep(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        Beep(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        //Spool for a total of 1 second
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        //Fire a burst shot
        Beep(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        Beep(false);

        Fire(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        Fire(false);
        Spool(false);

        //Give a 5 second cool down
        Beep(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        Beep(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        LOG->debug("Cooling down for 5 seconds...");
        std::this_thread::sleep_for(std::chrono::seconds(5));

        return PREV_STATE;
    }
    
    return 0;
}


/* Move toy vertically
 * Value [UP 1, STOP 0, DOWN -1]
*/
void MoveServo(int DIR)
{
    int static position = 230;

    if (DIR == UP)
    {
        if (position > MIN_SERVO)
            position--;
    }
    else if (DIR == DOWN)
    {
        if (position < MAX_SERVO)
            position++;
    }
    pwmWrite(SERVO_WP_PIN, position);
}


/* Move toy horizontally
 * Value ranges between 0 to 100
*/
void MoveDCMotor(int value, bool DIR)
{
    if (value == 0) //stop
    {
        //LOG->debug("Stopping DC motor");
        softPwmWrite(DC_MOTOR_MOV_PIN, 0);
        softPwmWrite(DC_MOTOR_DIR_PIN, 0);
    }
    else if (DIR == LEFT)
    {
        //LOG->debug("Moving DC motor left");
        softPwmWrite(DC_MOTOR_MOV_PIN, 0);
        softPwmWrite(DC_MOTOR_DIR_PIN, value);
    }
    else if (DIR == RIGHT)
    {
        //LOG->debug("Moving DC motor right");
        softPwmWrite(DC_MOTOR_MOV_PIN, value);
        softPwmWrite(DC_MOTOR_DIR_PIN, 0);
    }
}

void Beep(bool on)
{
    if (on)
    {
        LOG->debug("Beep on!");
        digitalWrite(BEEPER_PIN, 1);
    }
    else
    {
        LOG->debug("Beep off");
        digitalWrite(BEEPER_PIN, 0);
    }
}

void Spool(bool on)
{
    if (on)
    {
        LOG->debug("Spooling!");
        if (RUN_MANUAL || !NO_FIRE)
            digitalWrite(SPOOL_PIN, 1);
    }
    else
    {
        LOG->debug("Spooling off!");
        digitalWrite(SPOOL_PIN, 0);
    }
}

void Fire(bool on)
{
    if (on)
    {
        LOG->debug("Fire!");
        if (RUN_MANUAL || !NO_FIRE)
            digitalWrite(FIRE_PIN, 1);
    }
    else
    {
        LOG->debug("Fire off");
        digitalWrite(FIRE_PIN, 0);
    }
}

void LAZER(bool on)
{
    if (on)
    {
        LOG->debug("LAZER on");
        digitalWrite(LAZER_PIN, 1);
    }
    else
    {
        LOG->debug("LAZER off");
        digitalWrite(LAZER_PIN, 0);
    }
}



/* Follow the closest target
*/
void FollowObjectThread()
{
    try
    {
        LOG->debug("Starting follow thread!");
        int error = 0;
        int last_error = 0;
        int derivative = 0;
        int control = 0;
        int tolerance = 10;
        int servo_step = 0;

        float dist = 1.0;
        target_info t;
        t.target = 0;
        t.x = 0;
        t.y = 0;
        while(!STOP_FOLLOW_OBJ)
        {
            if(!GetClosestTarget(&t, &dist))
            {
                //Update distance
                TARGET_DIST = dist;

                //Implement PID control
                error = (CENTER_X - (int)t.x);
                derivative = error - last_error;
                control = (int)((Kp * error) + (Kd * derivative));
                
                sprintf(message, "Error=%5d  Last E=%5d  Derivative=%5d  Control=%5d  T=%d (%3d,%3d)  Dist=%.2f m",
                    error,last_error,derivative,control,t.target,t.x,t.y,TARGET_DIST);
                LOG->debug(message);

                //Limit speed
                if (control > 100) control = 100;
                if (control < -100) control = -100;
                last_error = error;

                if (control > 0)
                {
                    //Move left
                    if (control > tolerance)
                        MoveDCMotor(control, 0);
                }
                else if (control < 0)
                {
                    //Move right
                    if (control < -1*tolerance)
                        MoveDCMotor(-1*control, 1);
                }
                else
                    MoveDCMotor(STOP);


                //Move Down
                if (servo_step >= 6)
                {
                    servo_step = 0;
                    if (t.y < (CENTER_Y-PIXEL_RADIUS))
                    {
                        MoveServo(UP);
                    }
                    //Move Up
                    else if ((CENTER_Y+PIXEL_RADIUS) < t.y)
                    {
                        MoveServo(DOWN);
                    }
                }
                servo_step++;
            }
            else
            {
                TARGET_DIST = -1.0;
                error = 0;
                derivative = 0;
                control = 0;
                MoveDCMotor(STOP);
            }
        }
    }
    catch(const std::exception& e)
    {
        sprintf(message, e.what());
        LOG->error(message);
    }

    LOG->debug("Exiting follow thread!");
}


void StopFollowObjThread()
{
    //Destroy the follow thread if running
    if (STOP_FOLLOW_OBJ == false)
    {
        STOP_FOLLOW_OBJ = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (follow_thread.joinable())
        {
            follow_thread.join();
        }
    }
}
