# Variables
CC = gcc
CFLAGS = -Wall -pthread -O2
TARGET = 1901042671_main
SRCS = 1901042671_main.c
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean