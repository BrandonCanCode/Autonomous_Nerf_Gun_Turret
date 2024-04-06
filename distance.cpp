#include "distance.h"

//Changeable
#define T_TIMEOUT 10

//Shared global
extern std::shared_ptr<spdlog::logger> LOG;

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
float TARGET_DIST = -1.0;
int target_timeouts[MAX_TARGETS];
target_info last_target;

/* Function calls to librealsense may raise errors of type rs_error*/
void check_error(rs2_error* e)
{
    char mes[256];
    if (e)
    {
        sprintf(mes, "rs_error was raised when calling %s(%s):", rs2_get_failed_function(e), rs2_get_failed_args(e));
        LOG->error(mes);
        sprintf(mes, "    %s", rs2_get_error_message(e));
        LOG->error(mes);
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
    char mes[256];

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
    
    sprintf(mes, "There are %d connected RealSense devices.\n", dev_count);
    LOG->debug(mes);
    if (0 == dev_count)
        return;

    // Get the first connected device
    // The returned object should be released with rs2_delete_device(...)
    LOG->debug("Creating realsense device...");
    dev = rs2_create_device(device_list, 0, &e);
    check_error(e);

    print_device_info(dev);

    // Create a pipeline to configure, start and stop camera streaming
    // The returned object should be released with rs2_delete_pipeline(...)
    LOG->debug("Creating realsense pipeline...");
    pipeline = rs2_create_pipeline(ctx, &e);
    check_error(e);

    // Create a config instance, used to specify hardware configuration
    // The retunred object should be released with rs2_delete_config(...)
    LOG->debug("Creating realsense config...");
    config = rs2_create_config(&e);
    check_error(e);

    // Request a specific configuration
    LOG->debug("Applying realsense configuration to the stream...");
    rs2_config_enable_stream(config, RS2_STREAM_DEPTH, 0, WIDTH, HEIGHT, RS2_FORMAT_Z16, 30, &e);
    check_error(e);

    // Start the pipeline streaming
    // The retunred object should be released with rs2_delete_pipeline_profile(...)
    LOG->debug("Starting realsense pipeline...");
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
        target_timeouts[i] = T_TIMEOUT;
    }

    last_target.target = 0;
    last_target.x = 0;
    last_target.y = 0;

    //Start John's Object Detection (Python script)
    LOG->debug("Starting object detection script...");
    system("nohup python3 no_cam.py &");
}


void DestructDist()
{
    LOG->debug("Destroying distance camera");

    // Stop the pipeline streaming
    rs2_pipeline_stop(pipeline, &e);
    check_error(e);

    // Release resources
    LOG->debug("Releasing camera resources");
    rs2_delete_pipeline_profile(pipeline_profile);
    rs2_delete_config(config);
    rs2_delete_pipeline(pipeline);
    rs2_delete_device(dev);
    rs2_delete_device_list(device_list);
    rs2_delete_context(ctx);

    //Stop John's Object Detection (python script)
    LOG->debug("Killing object detection script");
    system("pkill -f no_cam.py &");
    
    // Detach and destroy the shared memory segment
    LOG->debug("Destroying shared memory");
    shmdt(shared_mem); 
    shmctl(shmid, IPC_RMID, NULL);
    
    LOG->debug("Exiting distance library...");
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
                // printf(message, "Target %d (%d,%d) is %.3f meters away.\n", 
                //     shared_mem[j].target, shared_mem[j].x, shared_mem[j].y, distances[j]);
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
        TARGET_DIST = -1.0;
        return 1;
    }
    else
    {
        if (t->target != shared_mem[index].target)
        {
            //Keep track of target to average out target switching
            target_timeouts[t->target]--;
            printf("Target %d timeout %d\n", t->target, target_timeouts[t->target]);

            //Only switch targets if timeout expires
            if (target_timeouts[t->target] < 0)
            {
                t->target = shared_mem[index].target;
                t->x = shared_mem[index].x;
                t->y = shared_mem[index].y;
                TARGET_DIST = close_distance;
            }
        }
        else
        {
            //Reset timeout
            target_timeouts[t->target] = T_TIMEOUT;

            //Update values
            t->x = shared_mem[index].x;
            t->y = shared_mem[index].y;
            TARGET_DIST = close_distance;
        }
    }

    return 0;
}