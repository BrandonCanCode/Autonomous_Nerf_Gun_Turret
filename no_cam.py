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



def capture_frames(video_device,targets_info,engine,show_image):
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
            
            

            rgb_frame = Image.fromarray(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
            poses, inference_time = engine.DetectPosesInImage(rgb_frame)
         #   print('Inference time: %.f ms' % (inference_time * 1000))
            target_num=0
            for pose in poses:
                if pose.keypoints[5].score >= 0.4 and pose.keypoints[6].score >= 0.4:
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

                    length = 15
                    center = (int(avg_kp_x),int(avg_kp_y))
                    # Draw horizontal line
                    cv2.line(frame, (center[0] - length, center[1]), (center[0] + length, center[1]), (0, 255, 0), 2)
 #                   cv2.circle(frame,center,length,(0,200,0),thickness=2)
                    # Draw vertical line
                    cv2.line(frame, (center[0], center[1] - length), (center[0], center[1] + length), (0, 255, 0), 2)

                    # Draw target number next to the horizontal line
                    cv2.putText(frame, str(target_num), (center[0] + length + 5, center[1]), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

                    # Update the target_info
                    target_infos[target_num] = (target_num, int(avg_kp_x-1), int(avg_kp_y-1))
                    target_num = target_num + 1 
                    # Pack up the info
                    new_target_infos_bytes = b"".join(struct.pack("III", *target_info) for target_info in target_infos)

                    # Write the updated target_info structs to shared memory
                    shm.write(new_target_infos_bytes)

                    #print(f"Updated target {target_num}: x={avg_kp_x}, y={avg_kp_y}")
                # Break the loop if 'q' is pressed
               
	 # Draw horizontal line
            length2 = 20
            center2 = (int(frame.shape[1]/2), int(frame.shape[0]/2))  # corrected the parentheses placement

            # Draw horizontal line
            cv2.line(frame, (center2[0] - length2, center2[1]), (center2[0] + length2, center2[1]), (0, 0, 255), 2)  # corrected the parentheses placement

            # Draw vertical line
            cv2.line(frame, (center2[0], center2[1] - length2), (center2[0], center2[1] + length2), (0, 0, 255), 2)  # corrected the parentheses placement

            if (show_image):
                cv2.imshow('Frame',frame)

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

    # Create an ArgumentParser object
    parser = argparse.ArgumentParser(description="A script with a command-line argument")

    # Add a positional argument for the filename
    parser.add_argument("show_image", nargs='?', default=None, help="Show image flag")

    # Parse the command-line arguments
    args = parser.parse_args()
    show_image = False
    if (args.show_image == 's'):
        show_image = True
    
    print(f"Show Image = {show_image}")
    
    target_num =0
    target_infos_bytes = shm.read(5 * struct.calcsize("III"))
        # Unpack target_info structs
    target_infos = [struct.unpack("III", target_infos_bytes[i*12:(i+1)*12]) for i in range(5)]
    engine = PoseEngine('posenet_mobilenet_v1_075_481_641_quant_decoder_edgetpu.tflite')
    capture_frames(video_device,target_infos,engine,show_image)
