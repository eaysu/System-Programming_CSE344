# Compiler
CC = gcc-13

# Compiler flags
CFLAGS = -Wall -pthread

# Valgrind flags
VALGRIND_FLAGS = --leak-check=full

# Target executable
TARGET = hw3

# Source files
SRCS = hw3.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Run Valgrind on the program
valgrind: $(TARGET)
	valgrind $(VALGRIND_FLAGS) ./$(TARGET)

# Phony targets
.PHONY: all clean run valgrind
