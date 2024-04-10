# Autonomous_Nerf_Gun_Turret
Utilizes an intel depth camera and a raspberry pi 4b to control an autonomous nerf gun turret (ANGT).
User's can also manually operate the ANGT with a bluetooth game controller.

# Dependencies
## SPD Logger
`sudo apt install libspdlog-dev`

## WiringPi
https://github.com/WiringPi/WiringPi

## RealSense SDK 2.0
https://github.com/IntelRealSense/librealsense

# Notes
To fix running camera with sudo, run `sudo xauth merge ~/.Xauthority`
