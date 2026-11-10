#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "logger.h"

#ifdef _WIN32
#define LOGGER_THREAD_RETURN DWORD WINAPI
#define LOGGER_THREAD_ARGUMENT LPVOID
#define logger_lock(logger) EnterCriticalSection(&(logger)->mutex)
#define logger_unlock(logger) LeaveCriticalSection(&(logger)->mutex)
#define logger_wait(condition, logger) SleepConditionVariableCS((condition), &(logger)->mutex, INFINITE)
#define logger_signal(condition) WakeConditionVariable((condition))
#define logger_broadcast(condition) WakeAllConditionVariable((condition))
#else
#define LOGGER_THREAD_RETURN void *
#define LOGGER_THREAD_ARGUMENT void *
#define logger_lock(logger) pthread_mutex_lock(&(logger)->mutex)
#define logger_unlock(logger) pthread_mutex_unlock(&(logger)->mutex)
#define logger_wait(condition, logger) pthread_cond_wait((condition), &(logger)->mutex)
#define logger_signal(condition) pthread_cond_signal((condition))
#define logger_broadcast(condition) pthread_cond_broadcast((condition))
#endif

static LOGGER_THREAD_RETURN logger_worker(LOGGER_THREAD_ARGUMENT argument)
{
    ActivityLogger *logger = argument;

    while (1)
    {
        char message[LOG_MESSAGE_SIZE];
        time_t now;
        struct tm *time_info;

        logger_lock(logger);
        while (logger->count == 0 && !logger->stopping)
        {
            logger_wait(&logger->not_empty, logger);
        }

        if (logger->count == 0 && logger->stopping)
        {
            logger_unlock(logger);
            break;
        }

        strcpy(message, logger->messages[logger->front]);
        logger->front = (logger->front + 1) % LOG_QUEUE_CAPACITY;
        logger->count--;
        logger_signal(&logger->not_full);
        logger_unlock(logger);

        now = time(NULL);
        time_info = localtime(&now);
        fprintf(logger->file, "[%02d:%02d:%02d] %s\n", time_info->tm_hour,
                time_info->tm_min, time_info->tm_sec, message);
        fflush(logger->file);
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int logger_start(ActivityLogger *logger, const char *file_name)
{
    memset(logger, 0, sizeof(*logger));
    logger->file = fopen(file_name, "a");
    if (logger->file == NULL)
    {
        perror("Cannot open activity log");
        return -1;
    }

#ifdef _WIN32
    InitializeCriticalSection(&logger->mutex);
    InitializeConditionVariable(&logger->not_empty);
    InitializeConditionVariable(&logger->not_full);
    logger->thread = CreateThread(NULL, 0, logger_worker, logger, 0, NULL);
    if (logger->thread == NULL)
    {
        fprintf(stderr, "Could not create logger thread.\n");
        fclose(logger->file);
        DeleteCriticalSection(&logger->mutex);
        return -1;
    }
#else
    if (pthread_mutex_init(&logger->mutex, NULL) != 0)
    {
        fprintf(stderr, "Could not initialize logger synchronization objects.\n");
        fclose(logger->file);
        return -1;
    }
    if (pthread_cond_init(&logger->not_empty, NULL) != 0)
    {
        fprintf(stderr, "Could not initialize logger synchronization objects.\n");
        fclose(logger->file);
        pthread_mutex_destroy(&logger->mutex);
        return -1;
    }
    if (pthread_cond_init(&logger->not_full, NULL) != 0)
    {
        fprintf(stderr, "Could not initialize logger synchronization objects.\n");
        fclose(logger->file);
        pthread_cond_destroy(&logger->not_empty);
        pthread_mutex_destroy(&logger->mutex);
        return -1;
    }
    if (pthread_create(&logger->thread, NULL, logger_worker, logger) != 0)
    {
        fprintf(stderr, "Could not create logger thread.\n");
        fclose(logger->file);
        pthread_mutex_destroy(&logger->mutex);
        pthread_cond_destroy(&logger->not_empty);
        pthread_cond_destroy(&logger->not_full);
        return -1;
    }
#endif

    return 0;
}

void logger_log(ActivityLogger *logger, const char *format, ...)
{
    va_list arguments;

    logger_lock(logger);
    while (logger->count == LOG_QUEUE_CAPACITY && !logger->stopping)
    {
        logger_wait(&logger->not_full, logger);
    }

    if (logger->stopping)
    {
        logger_unlock(logger);
        return;
    }

    va_start(arguments, format);
    vsnprintf(logger->messages[logger->rear], LOG_MESSAGE_SIZE, format, arguments);
    va_end(arguments);
    logger->rear = (logger->rear + 1) % LOG_QUEUE_CAPACITY;
    logger->count++;
    logger_signal(&logger->not_empty);
    logger_unlock(logger);
}

void logger_stop(ActivityLogger *logger)
{
    logger_lock(logger);
    logger->stopping = 1;
    logger_broadcast(&logger->not_empty);
    logger_broadcast(&logger->not_full);
    logger_unlock(logger);

#ifdef _WIN32
    WaitForSingleObject(logger->thread, INFINITE);
    CloseHandle(logger->thread);
#else
    pthread_join(logger->thread, NULL);
#endif
    fclose(logger->file);

#ifdef _WIN32
    DeleteCriticalSection(&logger->mutex);
#else
    pthread_mutex_destroy(&logger->mutex);
    pthread_cond_destroy(&logger->not_empty);
    pthread_cond_destroy(&logger->not_full);
#endif
}
