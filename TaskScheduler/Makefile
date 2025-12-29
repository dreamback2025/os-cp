# Makefile for Task Scheduler Simulator
# This is for cross-compilation or Windows environments

CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = -luser32 -lkernel32

# For Windows (MinGW/MSYS2)
TARGET = scheduler_simulator.exe
SRC = scheduler_simulator.c

# Default target
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Clean build artifacts
clean:
	del $(TARGET)
	
# Alternative clean for Unix-like environments
clean-unix:
	rm -f $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run