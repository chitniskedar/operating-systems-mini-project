#ifndef WORKER_H
#define WORKER_H

/* How a simulated worker should behave */
typedef enum
{
    WORKER_NORMAL,        /* Does its work and exits with status 0 */
    WORKER_EXIT_ERROR,    /* Exits with a non-zero status */
    WORKER_CRASH          /* Killed by a signal (abnormal termination) */
} WorkerMode;

void worker_run(int id, WorkerMode mode);   /* Runs inside the child */
void worker_report_status(int id, int pid, int status);
void worker_demo(void);

#endif
