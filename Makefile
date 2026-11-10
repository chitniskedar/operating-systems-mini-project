CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -Iinclude
TARGET = campuscompute
SOURCES = src/main.c src/scheduler.c src/logger.c

ifeq ($(OS),Windows_NT)
THREAD_FLAGS =
else
THREAD_FLAGS = -pthread
endif

all: $(TARGET)

$(TARGET): $(SOURCES) include/job.h include/logger.h include/scheduler.h
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET) campuscompute.log
