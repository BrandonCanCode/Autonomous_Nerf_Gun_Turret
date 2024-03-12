# makefile for the control library
CC = g++
CFLAGS = -Wall -DSPDLOG_COMPILED_LIB -lspdlog -pthread -l wiringPi

SRCS = test_control_main.cpp control.cpp
OUT = test_control_main

all:
	$(CC) $(SRCS) -o $(OUT) $(CFLAGS)

clean:
	rm -f $(OUT)
