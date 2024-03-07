/* main.cpp
 * Main loop that runs on the Raspberry Pi 5.
*/

#include "control_lib/control.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <unistd.h>
#include <stdbool.h>
// #include <linux/joystick.h>
// #include <thread>
// #include <fcntl.h>

//States
#define IDLE 0
#define OBJ_DETECT 1
#define TARGET_WARNING 2
#define TARGET_FIRE 3

//Joystick buttons and sticks (axis)
// #define BTN_TOGGLE_MODE 10
// #define BTN_BEEP 1
// #define AXIS_HORZONTAL 0
// #define AXIS_VERTICAL 3
// #define AXIS_SPOOL 4
// #define AXIS_FIRE 5

//Global
// bool RUN_MANUAL = false;
// int JOYSTICK_FD = -1;

std::shared_ptr<spdlog::logger> InitializeLogger();
// void JoyStickControl(control_lib *CL);


int main(int argc, char **argv)
{
    //Initialization
    std::shared_ptr<spdlog::logger> logger = InitializeLogger();
    logger->debug("System initializing...");
    control_lib CL(logger);

    // //Initialize joystick
    // JOYSTICK_FD = open("/dev/input/js0", O_RDONLY);
    // if (JOYSTICK_FD == -1)
    // {
    //     logger->error("Unable to open joystick...");
    // }

    //Startup joystick controller thread
    //std::thread js_thread(JoyStickControl, &CL);

    int state = IDLE;
    bool loop = true;
    while(loop)
    {
        // if (!RUN_MANUAL)
        // {
            if      (state == IDLE)             state += CL.RunIdle();
            else if (state == OBJ_DETECT)       state += CL.RunObjDetect();
            else if (state == TARGET_WARNING)   state += CL.RunTargetWarn();
            else if (state == TARGET_FIRE)      state += CL.RunTargetFire();
            else
            {
                //Should never occur
                logger->error("Invalid state %d!", state);
                loop = false;
            }
        // }
        // else sleep(1); //Wait for a mode toggle
    }

    logger->flush();
    return 0;
}


/* Function to initialize the logger with console and file sinks
*/
std::shared_ptr<spdlog::logger> InitializeLogger() 
{
    //Create a stdout sink (console output with color support)
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    //Create a file sink (logs to a file)
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/logfile.log", true); // Append to file if it exists

    // Create a logger with both console and file sinks
    auto logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list({console_sink, file_sink}));

    //Set the log level for both sinks and register
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);

    //Set desired settings
    spdlog::flush_every(std::chrono::seconds(3));
    spdlog::set_pattern("[%H:%M:%S %^%l%$] %v");

    return logger;
}



/* Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
// int read_event(int fd, struct js_event *event)
// {
//     ssize_t bytes;

//     bytes = read(fd, event, sizeof(*event));

//     if (bytes == sizeof(*event))
//         return 0;

//     /* Error, could not read full event. */
//     return -1;
// }

/* Current state of an axis.
 */
// struct axis_state 
// {
//     short x, y;
// };

/* Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
// size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
// {
//     size_t axis = event->number / 2;

//     if (axis < 3)
//     {
//         if (event->number % 2 == 0)
//             axes[axis].x = event->value;
//         else
//             axes[axis].y = event->value;
//     }

//     return axis;
// }

/* Manual control via a bluetooth controller.
 * '-' sign toggles between manual or AI controlling mode.
*/
// void JoyStickControl(control_lib *CL)
// {
//     if (JOYSTICK_FD != -1)
//     {
//         struct js_event event;
//         struct axis_state axes[3] = {0};
//         size_t axis;

//         // This loop will exit if the controller is unplugged.
//         while (read_event(JOYSTICK_FD, &event) == 0)
//         {
//             if (event.type == JS_EVENT_BUTTON && event.number == BTN_TOGGLE_MODE && event.value == true)
//             {
//                 RUN_MANUAL = !RUN_MANUAL;
//             }

//             if (RUN_MANUAL)
//             {
//                 //BUTTONS
//                 if (event.type == JS_EVENT_BUTTON && event.value == true) //Pressed button
//                 {
//                     printf("Button %u %s\n", event.number, event.value ? "pressed" : "released");
//                     if(event.number == BTN_BEEP)
//                     {
//                         //BEEP;
//                     }
//                 }
//                 //AXIS
//                 else if (event.type == JS_EVENT_AXIS)
//                 {
//                     axis = get_axis_state(&event, axes);
//                     printf("Axis %zu at (%6d, %6d) %u\n", axis, axes[axis].x, axes[axis].y, event.number);

//                     if (event.number == AXIS_HORZONTAL) 
//                         CL->move_stepper((axes[axis].x > 0 ? 1 : -1));
//                     if (event.number == AXIS_VERTICAL)
//                         CL->move_servo((axes[axis].y > 0 ? 1 : -1));
//                 }
//             }
//         }
//     }
// }