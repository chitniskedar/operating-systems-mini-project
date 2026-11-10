#include <stdio.h>
#include <string.h>
#include "scheduler.h"

static void prepare(Job jobs[], int count)
{
    int index;

    for (index = 0; index < count; index++)
    {
        jobs[index].status = JOB_READY;
        jobs[index].completion_time = 0;
        jobs[index].turnaround_time = 0;
        jobs[index].waiting_time = 0;
    }
}

static int earlier_arrival(const Job *first, const Job *second)
{
    if (first->arrival_time != second->arrival_time)
    {
        return first->arrival_time < second->arrival_time;
    }
    return first->id < second->id;
}

static void show_results(Job jobs[], int count)
{
    int index;
    double total_turnaround = 0;
    double total_waiting = 0;

    printf("\n%-6s %-8s %-11s %-8s %-11s %-11s %-8s\n", "Job", "Arrival", "CPU Burst", "Priority", "Completion", "Turnaround", "Waiting");
    for (index = 0; index < count; index++)
    {
        jobs[index].turnaround_time = jobs[index].completion_time - jobs[index].arrival_time;
        jobs[index].waiting_time = jobs[index].turnaround_time - jobs[index].cpu_burst;
        total_turnaround += jobs[index].turnaround_time;
        total_waiting += jobs[index].waiting_time;
        printf("J%-5d %-8d %-11d %-8d %-11d %-11d %-8d\n", jobs[index].id,
               jobs[index].arrival_time, jobs[index].cpu_burst, jobs[index].priority,
               jobs[index].completion_time, jobs[index].turnaround_time, jobs[index].waiting_time);
    }
    printf("Average turnaround time: %.2f\n", total_turnaround / count);
    printf("Average waiting time: %.2f\n", total_waiting / count);
}

void run_fcfs(Job jobs[], int count, ActivityLogger *logger)
{
    int used[MAX_JOBS] = {0};
    int completed = 0;
    int current_time = 0;

    prepare(jobs, count);
    printf("\nFCFS timeline: ");
    while (completed < count)
    {
        int best = -1;
        int index;
        for (index = 0; index < count; index++)
        {
            if (!used[index] && (best == -1 || earlier_arrival(&jobs[index], &jobs[best])))
            {
                best = index;
            }
        }
        if (current_time < jobs[best].arrival_time)
        {
            printf("Idle[%d-%d] ", current_time, jobs[best].arrival_time);
            current_time = jobs[best].arrival_time;
        }
        printf("J%d[%d-%d] ", jobs[best].id, current_time, current_time + jobs[best].cpu_burst);
        logger_log(logger, "FCFS: J%d ran from %d to %d", jobs[best].id, current_time, current_time + jobs[best].cpu_burst);
        current_time += jobs[best].cpu_burst;
        jobs[best].completion_time = current_time;
        jobs[best].status = JOB_COMPLETED;
        used[best] = 1;
        completed++;
    }
    printf("\nTie-breaker: earlier arrival time, then lower job ID.\n");
    show_results(jobs, count);
}

void run_priority(Job jobs[], int count, ActivityLogger *logger)
{
    int completed = 0;
    int current_time = 0;

    prepare(jobs, count);
    printf("\nNon-preemptive Priority timeline: ");
    while (completed < count)
    {
        int best = -1;
        int index;
        for (index = 0; index < count; index++)
        {
            if (jobs[index].status == JOB_READY && jobs[index].arrival_time <= current_time &&
                (best == -1 || jobs[index].priority < jobs[best].priority ||
                 (jobs[index].priority == jobs[best].priority && earlier_arrival(&jobs[index], &jobs[best]))))
            {
                best = index;
            }
        }
        if (best == -1)
        {
            int next = -1;
            for (index = 0; index < count; index++)
            {
                if (jobs[index].status == JOB_READY && (next == -1 || earlier_arrival(&jobs[index], &jobs[next])))
                {
                    next = index;
                }
            }
            printf("Idle[%d-%d] ", current_time, jobs[next].arrival_time);
            current_time = jobs[next].arrival_time;
            continue;
        }
        printf("J%d[%d-%d] ", jobs[best].id, current_time, current_time + jobs[best].cpu_burst);
        logger_log(logger, "Priority: J%d (priority %d) ran from %d to %d", jobs[best].id, jobs[best].priority, current_time, current_time + jobs[best].cpu_burst);
        current_time += jobs[best].cpu_burst;
        jobs[best].completion_time = current_time;
        jobs[best].status = JOB_COMPLETED;
        completed++;
    }
    printf("\nLower numeric priority value runs first. Tie-breaker: earlier arrival, then lower job ID.\n");
    show_results(jobs, count);
}

void run_round_robin(Job jobs[], int count, int quantum, ActivityLogger *logger)
{
    int remaining[MAX_JOBS];
    int order[MAX_JOBS];
    int queue[MAX_JOBS];
    int head = 0, tail = 0, queued = 0, completed = 0, current_time = 0, next_arrival = 0;
    int index;

    prepare(jobs, count);
    for (index = 0; index < count; index++)
    {
        remaining[index] = jobs[index].cpu_burst;
        order[index] = index;
    }
    for (index = 0; index < count - 1; index++)
    {
        int candidate;
        for (candidate = index + 1; candidate < count; candidate++)
        {
            if (earlier_arrival(&jobs[order[candidate]], &jobs[order[index]]))
            {
                int temporary = order[index];
                order[index] = order[candidate];
                order[candidate] = temporary;
            }
        }
    }
    printf("\nRound Robin timeline: ");
    while (completed < count)
    {
        if (queued == 0)
        {
            if (current_time < jobs[order[next_arrival]].arrival_time)
            {
                printf("Idle[%d-%d] ", current_time, jobs[order[next_arrival]].arrival_time);
                current_time = jobs[order[next_arrival]].arrival_time;
            }
            while (next_arrival < count && jobs[order[next_arrival]].arrival_time <= current_time)
            {
                queue[tail] = order[next_arrival++];
                tail = (tail + 1) % MAX_JOBS;
                queued++;
            }
        }
        index = queue[head];
        head = (head + 1) % MAX_JOBS;
        queued--;
        {
            int run = remaining[index] < quantum ? remaining[index] : quantum;
            printf("J%d[%d-%d] ", jobs[index].id, current_time, current_time + run);
            logger_log(logger, "Round Robin: J%d ran from %d to %d", jobs[index].id, current_time, current_time + run);
            current_time += run;
            remaining[index] -= run;
        }
        while (next_arrival < count && jobs[order[next_arrival]].arrival_time <= current_time)
        {
            queue[tail] = order[next_arrival++];
            tail = (tail + 1) % MAX_JOBS;
            queued++;
        }
        if (remaining[index] == 0)
        {
            jobs[index].completion_time = current_time;
            jobs[index].status = JOB_COMPLETED;
            completed++;
        }
        else
        {
            queue[tail] = index;
            tail = (tail + 1) % MAX_JOBS;
            queued++;
        }
    }
    printf("\nTime quantum: %d. Arrival tie-breaker: lower job ID.\n", quantum);
    show_results(jobs, count);
}
