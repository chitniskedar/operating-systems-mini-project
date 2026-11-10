#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#define LOG_QUEUE_CAPACITY 32
#define LOG_MESSAGE_SIZE 160

typedef struct
{
    char messages[LOG_QUEUE_CAPACITY][LOG_MESSAGE_SIZE];
    int front;
    int rear;
    int count;
    int stopping;
#ifdef _WIN32
    CRITICAL_SECTION mutex;
    CONDITION_VARIABLE not_empty;
    CONDITION_VARIABLE not_full;
    HANDLE thread;
#else
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_t thread;
#endif
    FILE *file;
} ActivityLogger;

int logger_start(ActivityLogger *logger, const char *file_name);
void logger_log(ActivityLogger *logger, const char *format, ...);
void logger_stop(ActivityLogger *logger);

#endif
