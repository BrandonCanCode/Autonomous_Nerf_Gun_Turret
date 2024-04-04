import cv2
from pose_engine import PoseEngine
from PIL import Image
from PIL import ImageDraw
import sysv_ipc
import struct
import argparse
import numpy as np
import os

key = sysv_ipc.ftok("/tmp", 42)  # Use an integer value for proj_id

# Access the same shared memory segment
shm = sysv_ipc.SharedMemory(key, flags=sysv_ipc.IPC_CREAT, size=5 * struct.calcsize("III"))  # Assuming each target_info struct is 12 bytes (3 * uint32_t)



def find_rgb_camera():
    # Iterate through each video device until an RGB camera is found
    device_index = 0
    while True:
        video_dev = '/dev/video' + str(device_index)
        cap = cv2.VideoCapture(video_dev)
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        if cap.isOpened():
            # Check if the captured frame is in RGB format
            #ret, frame = cap.read()
            #if ret and frame is not None and frame.shape[2] == 3:
            print("Found camera at " + video_dev)
            cap.release()
            return device_index
        device_index += 1


def capture_frames(video_device,targets_info,engine):
    # Open video capture device
    cap = cv2.VideoCapture(video_device)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    # Check if the camera opened successfully
    if not cap.isOpened():
        print("Error: Unable to open camera")
        return

    try:
        # Main loop to capture frames
        while True:
            # Capture frame-by-frame
           
            ret, frame = cap.read()

            # Check if the frame was successfully captured
            if not ret:
                print("Error: Unable to capture frame")
                break
            
            
            # Display the frame
         #   cv2.imshow('Frame', frame)
            rgb_frame = Image.fromarray(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
            poses, inference_time = engine.DetectPosesInImage(rgb_frame)
         #   print('Inference time: %.f ms' % (inference_time * 1000))
            target_num=0
            for pose in poses:
                if pose.keypoints[5].score >= 0.3 and pose.keypoints[6].score >= 0.3:
                # print("Left and right keypoints are present")
                    kp_x_1 = int(pose.keypoints[5].point[0] )
                    kp_y_1 = int(pose.keypoints[5].point[1])

                    kp_x_2 = int(pose.keypoints[6].point[0])
                    kp_y_2 = int(pose.keypoints[6].point[1])

                    avg_kp_x = (kp_x_1 + kp_x_2) / 2
                    avg_kp_y = (kp_y_1 + kp_y_2) / 2

                    avg_kp_x = min(avg_kp_x, 640)
                    avg_kp_y = min(avg_kp_y, 640)

                    avg_kp_x = max(avg_kp_x, 1)
                    avg_kp_y = max(avg_kp_y, 1)

                    # Update the target_info
                    target_infos[target_num] = (target_num, int(avg_kp_x-1), int(avg_kp_y-1))
                    target_num = target_num + 1 
                    # Pack up the info
                    new_target_infos_bytes = b"".join(struct.pack("III", *target_info) for target_info in target_infos)

                    # Write the updated target_info structs to shared memory
                    shm.write(new_target_infos_bytes)

          #          print(f"Updated target {target_num}: x={avg_kp_x}, y={avg_kp_y}")
            # Break the loop if 'q' is pressed
            while target_num <5:
                target_infos[target_num] = (target_num, int(0),int(0))
                target_num = target_num + 1
                new_target_infos_bytes = b"".join(struct.pack("III", *target_info) for target_info in target_infos)

                # Write the updated target_info structs to shared memory
                shm.write(new_target_infos_bytes)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    finally:
        # Release the capture when done
        cap.release()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    video_device = '/dev/video4'
    #video_device = find_rgb_camera()
    print("Vid: " + str(video_device))
    target_num =0
    target_infos_bytes = shm.read(5 * struct.calcsize("III"))
        # Unpack target_info structs
    target_infos = [struct.unpack("III", target_infos_bytes[i*12:(i+1)*12]) for i in range(5)]
    engine = PoseEngine('posenet_mobilenet_v1_075_481_641_quant_decoder_edgetpu.tflite')
    capture_frames(video_device,target_infos,engine)
