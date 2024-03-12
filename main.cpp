/* main.cpp
 * Main loop that runs on the Raspberry Pi 5.
*/

#include "control_lib/control.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <unistd.h>
#include <stdbool.h>

//States
#define IDLE 0
#define OBJ_DETECT 1
#define TARGET_WARNING 2
#define TARGET_FIRE 3

//Globals
std::shared_ptr<spdlog::logger> InitializeLogger();


int main(int argc, char **argv)
{
    //Initialization
    std::shared_ptr<spdlog::logger> logger = InitializeLogger();
    logger->debug("System initializing...");
    InitCL(logger);

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
                logger->error("Invalid state!");
                loop = false;
            }
        }
        else sleep(1); //Wait for a mode toggle
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