# Autonomous Nerf Gun Turret
<img src="https://github.com/BrandonCanCode/Autonomous_Nerf_Gun_Turret/assets/69353222/2c996650-c540-498e-8d52-8a03894cf890" width="400" />

This project utilizes an intel depth camera, Raspberry Pi 4B, Bluetooth controller, TPU, motion sensors, and computer vision to operate an Autonomous Nerf Gun Turret (ANGT).

## Autonomous Mode
* AI runs on TensorFlow and OpenCV.
* The coral USB accelerator (TPU) speeds up the AI's inferences. 
* The autonomous system steps between the different states as seen below:

![system_states drawio](https://github.com/BrandonCanCode/Autonomous_Nerf_Gun_Turret/assets/69353222/bd28b0b1-9382-4382-8914-59a8d97457fa)

## Manual Mode
Users can also manually operate the ANGT with a Bluetooth game controller.
Functionality includes:
* Horizontal movement (DC motor)
* Vertical movement (servo motor)
* Gun spooling
* Gun firing
* Beeper (noisemaker)
* LASER (manual aiming only)

# Dependencies
## SPD Logger
`sudo apt install libspdlog-dev`

## WiringPi
https://github.com/WiringPi/WiringPi

## RealSense SDK 2.0
https://github.com/IntelRealSense/librealsense

## OpenCV (Python)
https://opencv.org/

## TensorFlow (Python)
https://www.tensorflow.org/

## Coral USB Accelerator
https://coral.ai/products/accelerator/

## Raspian OS (Bullseye)
This version is required to get the realsense2 library to make and function. This library still only works in the C++ language.

# Notes
* To fix the X11 forwarding issue with the camera and sudo, run `sudo xauth merge ~/.Xauthority`
* May also need to purge and reinstall Xauth
