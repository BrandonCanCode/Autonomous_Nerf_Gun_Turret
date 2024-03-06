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
enum state_e {
    IDLE,
    OBJ_DETECT,
    TARGET_WARNING,
    TARGET_FIRE
};

std::shared_ptr<spdlog::logger> InitializeLogger();


int main(int argc, char **argv)
{
    //Initialization
    std::shared_ptr<spdlog::logger> logger = InitializeLogger();
    logger->debug("System initializing...");
    control_lib CL(logger);

    state_e state = IDLE;

    bool loop = true;
    int result = 0;
    while(loop)
    {
        switch (state)
        {
        case IDLE:
            result = CL.RunIdle();
            if (result == NEXT_STATE)
                state = OBJ_DETECT;
            break;
        
        case OBJ_DETECT:
            result = CL.RunObjDetect();
            if (result == PREV_STATE)
                state = IDLE;
            else if (result == NEXT_STATE)
                state = TARGET_WARNING;
            break;
        
        case TARGET_WARNING:
            result = CL.RunTargetWarn();
            if (result == PREV_STATE)
                state = OBJ_DETECT;
            else if (result == NEXT_STATE)
                state = TARGET_FIRE;
            break;

        case TARGET_FIRE:
            result = CL.RunTargetFire();
            if (result == PREV_STATE)
                state = TARGET_WARNING;
            break;
        
        default: //Should never occur
            logger->error("Invalid state!");
            loop = false;
            break;
        }
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
