#ifndef JOB_H
#define JOB_H

#define MAX_JOBS 100

typedef enum
{
    JOB_READY,
    JOB_RUNNING,
    JOB_COMPLETED
} JobStatus;

typedef struct
{
    int id;
    int arrival_time;
    int cpu_burst;
    int priority;
    JobStatus status;
    int completion_time;
    int turnaround_time;
    int waiting_time;
} Job;

const char *job_status_text(JobStatus status);
void print_jobs(const Job jobs[], int count);

#endif
