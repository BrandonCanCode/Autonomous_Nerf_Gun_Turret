# makefile for the control library
CC = g++
CFLAGS = -Wall -DSPDLOG_COMPILED_LIB -lspdlog -pthread -l wiringPi -lrealsense2

SRCS = main.cpp control.cpp distance.cpp joystick.cpp
OUT = run

all:
	$(CC) $(SRCS) -o $(OUT) $(CFLAGS)

clean:
	rm -f $(OUT)
