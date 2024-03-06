/* test_control_main.cpp
 * Used to run tests on the control library.
*/

#include "control.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> InitializeLogger();


int main(int argc, char **argv)
{
    //Initialization
    std::shared_ptr<spdlog::logger> logger = InitializeLogger();
    logger->debug("System initializing...");
    control_lib CL(logger);



    logger->debug("Done!");
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