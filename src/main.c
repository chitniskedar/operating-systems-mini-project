#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "job.h"
#include "logger.h"
#include "scheduler.h"
#include "ipc.h"
#include "bankers.h"
#include "worker.h"
#include "rlimits.h"

static int read_integer(const char *prompt, int minimum, int maximum, int *value)
{
    char line[80];
    char *end;
    long parsed;

    printf("%s", prompt);
    if (fgets(line, sizeof(line), stdin) == NULL)
    {
        return 0;
    }
    errno = 0;
    parsed = strtol(line, &end, 10);
    while (*end == ' ' || *end == '\t')
    {
        end++;
    }
    if (errno != 0 || end == line || (*end != '\n' && *end != '\0') || parsed < minimum || parsed > maximum)
    {
        printf("Invalid input. Enter an integer from %d to %d.\n", minimum, maximum);
        return -1;
    }
    *value = (int)parsed;
    return 1;
}

const char *job_status_text(JobStatus status)
{
    if (status == JOB_READY)
    {
        return "Ready";
    }
    if (status == JOB_RUNNING)
    {
        return "Running";
    }
    return "Completed";
}

void print_jobs(const Job jobs[], int count)
{
    int index;

    if (count == 0)
    {
        printf("No jobs have been added.\n");
        return;
    }
    printf("\n%-6s %-8s %-11s %-8s %-10s\n", "Job", "Arrival", "CPU Burst", "Priority", "Status");
    for (index = 0; index < count; index++)
    {
        printf("J%-5d %-8d %-11d %-8d %-10s\n", jobs[index].id, jobs[index].arrival_time,
               jobs[index].cpu_burst, jobs[index].priority, job_status_text(jobs[index].status));
    }
}

static int add_job(Job jobs[], int *count, ActivityLogger *logger)
{
    Job *job;
    int result;

    if (*count == MAX_JOBS)
    {
        printf("Job limit (%d) reached.\n", MAX_JOBS);
        return 1;
    }
    job = &jobs[*count];
    job->id = *count + 1;
    result = read_integer("Arrival time (0-10000): ", 0, 10000, &job->arrival_time);
    if (result != 1) return result;
    result = read_integer("CPU burst (1-100): ", 1, 100, &job->cpu_burst);
    if (result != 1) return result;
    result = read_integer("Priority (1 is highest, 1-99): ", 1, 99, &job->priority);
    if (result != 1) return result;
    job->status = JOB_READY;
    job->completion_time = 0;
    job->turnaround_time = 0;
    job->waiting_time = 0;
    (*count)++;
    logger_log(logger, "Job J%d created: arrival=%d burst=%d priority=%d", job->id, job->arrival_time, job->cpu_burst, job->priority);
    printf("Added J%d.\n", job->id);
    return 1;
}

int main(void)
{
    Job jobs[MAX_JOBS];
    ActivityLogger logger;
    int count = 0;
    int choice;

    if (logger_start(&logger, "campuscompute.log") != 0)
    {
        return EXIT_FAILURE;
    }
    logger_log(&logger, "CampusCompute started");
    printf("CampusCompute: Job Scheduling and Activity Logger\n");

    while (1)
    {
        int result;

        printf("\n1. Add job\n2. List jobs\n3. Run FCFS\n4. Run Round Robin\n5. Run Priority\n"
               "6. IPC demo\n7. Banker's Algorithm\n8. Worker failure demo\n9. Resource limits demo\n10. Exit\n");
        result = read_integer("Choose an option: ", 1, 10, &choice);
        if (result == 0)
        {
            printf("\nEnd of input received. Cleaning up.\n");
            break;
        }
        if (result < 0)
        {
            continue;
        }
        if (choice == 1)
        {
            if (add_job(jobs, &count, &logger) == 0)
            {
                printf("\nEnd of input received. Cleaning up.\n");
                break;
            }
        }
        else if (choice == 2)
        {
            print_jobs(jobs, count);
        }
        else if (choice >= 3 && choice <= 5)
        {
            if (count == 0)
            {
                printf("Add at least one job before scheduling.\n");
            }
            else if (choice == 3)
            {
                run_fcfs(jobs, count, &logger);
            }
            else if (choice == 4)
            {
                int quantum;
                int result = read_integer("Round Robin time quantum (1-100): ", 1, 100, &quantum);
                if (result == 1)
                {
                    run_round_robin(jobs, count, quantum, &logger);
                }
                else if (result == 0)
                {
                    printf("\nEnd of input received. Cleaning up.\n");
                    break;
                }
            }
            else
            {
                run_priority(jobs, count, &logger);
            }
        }
        else if (choice == 6)
        {
            logger_log(&logger, "FR4 IPC demo started");
            ipc_demo();
        }
        else if (choice == 7)
        {
            logger_log(&logger, "FR5 Banker's demo started");
            bankers_demo();
        }
        else if (choice == 8)
        {
            logger_log(&logger, "FR6 worker demo started");
            worker_demo();
        }
        else if (choice == 9)
        {
            logger_log(&logger, "FR7 limits demo started");
            limits_demo();
        }
        else
        {
            logger_log(&logger, "CampusCompute exiting normally");
            break;
        }
    }
    logger_stop(&logger);
    return EXIT_SUCCESS;
}
