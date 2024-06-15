CC = gcc
CFLAGS = -Wall -Wextra
TARGET = hw2

SRCS = hw2.c

OBJS = $(SRCS:.c=.o)

# Define FIFO filenames
FIFO1 = /tmp/fifo1
FIFO2 = /tmp/fifo2s

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(FIFO1) $(FIFO2)
