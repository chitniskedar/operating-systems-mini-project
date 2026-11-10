CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude
TARGET = campuscompute

# Person 1: scheduling and logging
SOURCES = src/main.c src/scheduler.c src/logger.c
# Person 2: IPC, Banker's, worker failure, resource limits
SOURCES += src/ipc.c src/bankers.c src/worker.c src/rlimits.c

HEADERS = include/job.h include/logger.h include/scheduler.h \
          include/ipc.h include/bankers.h include/worker.h include/rlimits.h

# POSIX shared memory and semaphores need -lrt on Linux (macOS has no librt)
ifeq ($(shell uname -s),Linux)
LIBS = -lrt
else
LIBS =
endif

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -pthread $(SOURCES) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET) campuscompute.log
