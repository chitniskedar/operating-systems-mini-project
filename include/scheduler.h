#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "job.h"
#include "logger.h"

void run_fcfs(Job jobs[], int count, ActivityLogger *logger);
void run_priority(Job jobs[], int count, ActivityLogger *logger);
void run_round_robin(Job jobs[], int count, int quantum, ActivityLogger *logger);

#endif
