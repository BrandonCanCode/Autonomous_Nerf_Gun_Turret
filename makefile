# makefile for the control library
CC = g++
CFLAGS = -Wall -DSPDLOG_COMPILED_LIB -lspdlog

SRCS = main.cpp control_lib/control.cpp
OUT = run

all:
	$(CC) $(SRCS) -o $(OUT) $(CFLAGS)

clean:
	rm -f $(OUT)
