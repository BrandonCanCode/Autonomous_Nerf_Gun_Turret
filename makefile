CC = g++
CFLAGS = -Wall
LIBS = -DSPDLOG_COMPILED_LIB -lspdlog -pthread -lwiringPi -lrealsense2 -lfmt

SRCS = main.cpp joystick.cpp distance.cpp control.cpp
OBJDIR = obj
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
OUT = run

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJS) $(OUT)
