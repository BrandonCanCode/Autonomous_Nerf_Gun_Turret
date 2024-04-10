/* main.cpp
 * Main loop that runs on the Raspberry Pi 4b.
*/

#include "control.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <chrono>
#include <thread>

//States
#define IDLE 0
#define OBJ_DETECT 1
#define TARGET_WARNING 2
#define TARGET_FIRE 3

//Functions
std::shared_ptr<spdlog::logger> InitializeLogger();

//Globals
std::shared_ptr<spdlog::logger> LOG;


void SigHandle(int sig)
{
    if (sig == SIGINT || sig == SIGTERM || sig == SIGSEGV)
    {
        //Exiting 
        printf("\nExiting via sig handle...\n");
        DestructCL();
        exit(1);
    }
}

void printUsage(char **argv)
{
    printf("Usage: sudo %s [OPTIONS]\n", argv[0]);
    printf("[-p float] Proportional gain Kp\n");
    printf("[-d float] Derivative gain Kd\n");
    printf("[-n      ] No Fire (disable spool and fire for autonomous mode)\n");
    printf("[-s      ] Show camera image (X11 forwarding only)\n");
}


int main(int argc, char **argv)
{
    //Validate sudo
    if (geteuid() != 0)
    {
        fprintf(stderr, "You must run the program with 'sudo' privileges\n");
        return 1;
    }

    //Parse input args
    int opt;
    float Kp = 0.32; //Proportional Gain
    float Kd = 0.2; //Derivative Gain
    char *endptr;
    bool no_fire = false;
    bool show_image = false;

    // Parse command-line options using getopt
    while ((opt = getopt(argc, argv, "hp:d:ns")) != -1) 
    {
        switch (opt) {
            case 'p':
                Kp = strtof(optarg, &endptr);
                if (*endptr != '\0') 
                {
                    fprintf(stderr, "Invalid float value for option 'p' Proportional gain: %s\n", optarg);
                    return 1;
                }
                break;
            case 'd':
                Kd = strtof(optarg, &endptr);
                if (*endptr != '\0') 
                {
                    fprintf(stderr, "Invalid float value for option 'd' Derivative gain: %s\n", optarg);
                    return 1;
                }
                break;
            case 'n':
                no_fire = true;
                break;
            case 's':
                show_image = true;
                break;
            case 'h':
                printUsage(argv);
                return 1;
            default:
                printUsage(argv);
                return 1;
        }
    }

    //Setup signal handling
    signal(SIGINT, SigHandle);
    signal(SIGTERM, SigHandle);
    signal(SIGSEGV, SigHandle);

    //Initialization
    LOG = InitializeLogger();
    LOG->debug("System initializing...");
    InitCL(Kp, Kd, no_fire, show_image);

    int state = IDLE;
    bool loop = true;
    while(loop)
    {
        if (!RUN_MANUAL)
        {
            if      (state == IDLE)             state += RunIdle();
            else if (state == OBJ_DETECT)       state += RunObjDetect();
            else if (state == TARGET_WARNING)   state += RunTargetWarn();
            else if (state == TARGET_FIRE)      state += RunTargetFire();
            else
            {
                //Should never occur
                LOG->error("Invalid state!");
                loop = false;
            }
        }
        else 
        {
            std::this_thread::sleep_for(std::chrono::seconds(1)); //Wait for a mode toggle
            state = IDLE;
        }
    }

    LOG->flush();
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
