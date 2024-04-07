
# makefile for the control library
CC = g++
CFLAGS = -Wall -DSPDLOG_COMPILED_LIB -lspdlog -pthread -lwiringPi -lrealsense2

SRCS = main.cpp control.cpp distance.cpp joystick.cpp
OUT = run

all:
	$(CC) $(SRCS) -o $(OUT) $(CFLAGS)

test:
	$(CC) test_prog.cpp control.cpp -o test_out $(CFLAGS)

clean:
	rm -f $(OUT)
