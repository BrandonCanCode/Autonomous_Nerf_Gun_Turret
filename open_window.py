import cv2
import time

def find_rgb_camera():
    # Iterate through each video device until an RGB camera is found
    device_index = 0
    while True:
        cap = cv2.VideoCapture(device_index)
        if cap.isOpened():
            # Check if the captured frame is in RGB format
            ret, frame = cap.read()
            if ret and frame is not None and frame.shape[2] == 3:
                print(f"Found RGB camera at device index {device_index}.")
                return cap, device_index
            cap.release()
        device_index += 1

def main():
    while True:
        # Attempt to find an RGB camera
        cap, device_index = find_rgb_camera()
        if cap is None:
            print("Error: Unable to find an RGB camera.")
            return

        # Create a window
        cv2.namedWindow("Webcam", cv2.WINDOW_NORMAL)

        while True:
            # Capture frame-by-frame
            ret, frame = cap.read()

            # If frame is read correctly, display it
            if ret:
                # Draw a crosshair in the center
                height, width, _ = frame.shape
                center_x = width // 2
                center_y = height // 2
                cv2.line(frame, (center_x - 10, center_y), (center_x + 10, center_y), (0, 255, 0), 1)
                cv2.line(frame, (center_x, center_y - 10), (center_x, center_y + 10), (0, 255, 0), 1)

                cv2.imshow("Webcam", frame)

            # Check if the camera is still connected
            if not cap.isOpened():
                print("Camera disconnected. Attempting to find again...")
                break

            # Break the loop when 'q' is pressed
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

        # Close the window
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
