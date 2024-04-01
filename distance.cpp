#include "distance.h"
#include <cstdlib>

#define STREAM          RS2_STREAM_DEPTH  // rs2_stream is a types of data provided by RealSense device           
#define FORMAT          RS2_FORMAT_Z16    // rs2_format identifies how binary data is encoded within a frame      
#define WIDTH           640               // Defines the number of columns for each frame or zero for auto resolve
#define HEIGHT          0                 // Defines the number of lines for each frame or zero for auto resolve  
#define FPS             30                // Defines the rate of frames per second                                
#define STREAM_INDEX    0                 // Defines the stream index, used for multiple streams of the same type

//Globals
bool STOP;
extern std::shared_ptr<spdlog::logger> LOG;
std::thread thread;
int shmid;

target_info *shared_mem;
rs2_error* e;
rs2_context* ctx;
rs2_device_list* device_list;
rs2_device* dev;
rs2_pipeline* pipeline;
rs2_config* config;
rs2_pipeline_profile* pipeline_profile;


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
    STOP = false;
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
    rs2_config_enable_stream(config, STREAM, STREAM_INDEX, WIDTH, HEIGHT, FORMAT, FPS, &e);
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
    shmid = shmget(key, MAX_TARGETS*sizeof(target_info), IPC_CREAT | 0666); // Create a shared memory segment for 5 target_info structs
    shared_mem = (target_info *)shmat(shmid, NULL, 0); // Attach the shared memory segment
    //printf("shmid=%i\n", shmid);

    // Initialize target_info structs
    for (int i=0; i<MAX_TARGETS; i++) 
    {
        shared_mem[i].target = i; // Example: set target value
        shared_mem[i].x = 0;      // Initialize x coordinate
        shared_mem[i].y = 0;      // Initialize y coordinate
    }

    //Start thread
    thread = std::thread(RunDistanceThread);

    //Start John's Object Detection (Python script)
    system("nohup python3 no_cam.py &");
}


void DestructDist()
{
    LOG->debug("Destroying distance lib");
    STOP = true;

    // Stop the pipeline streaming
    rs2_pipeline_stop(pipeline, &e);
    check_error(e);

    // Release resources
    // rs2_delete_pipeline_profile(pipeline_profile);
    // rs2_delete_config(config);
    // rs2_delete_pipeline(pipeline);
    // rs2_delete_device(dev);
    // rs2_delete_device_list(device_list);
    // rs2_delete_context(ctx);

    //Stop John's Object Detection (python script)
    LOG->debug("Killing object detection script");
    system("pkill -f no_cam.py");
    
    // Detach and destroy the shared memory segment
    shmdt(shared_mem); 
    shmctl(shmid, IPC_RMID, NULL);
}


void RunDistanceThread()
{
    LOG->debug("Starting distance thread!");
    while (!STOP)
    {
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

            // Print the distance
            for(int j=0; j<MAX_TARGETS; j++)
            {
                if (shared_mem[j].x != 0 && shared_mem[j].y != 0)
                {
                    float distance = rs2_depth_frame_get_distance(frame, shared_mem[j].x, shared_mem[j].y, &e);
                    check_error(e);
                    printf("Target %d (%d,%d) is %.3f meters away.\n", 
                        shared_mem[j].target, shared_mem[j].x, shared_mem[j].y, distance);
                }
            }

            rs2_release_frame(frame);
        }

        rs2_release_frame(frames);
    }

    LOG->debug("Exiting distance thread!");
}
