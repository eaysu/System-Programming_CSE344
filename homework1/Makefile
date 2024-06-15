CC = gcc
CFLAGS = -Wall -Wextra -g

SRCS = gtuStudentGrades.c
OBJS = $(SRCS:.c=.o)
TARGET = gtuStudentGrades

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)
