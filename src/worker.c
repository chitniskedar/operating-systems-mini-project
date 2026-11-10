#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "worker.h"

#define NUM_WORKERS 4

void worker_run(int id, WorkerMode mode)
{
    printf("  [Worker %d | pid %d] started\n", id, (int)getpid());
    fflush(stdout);
    sleep(1);                         /* Pretend to do some work */

    if (mode == WORKER_EXIT_ERROR)
    {
        printf("  [Worker %d] simulated error, exiting with status 3\n", id);
        fflush(stdout);
        _exit(3);
    }
    if (mode == WORKER_CRASH)
    {
        printf("  [Worker %d] simulated crash (SIGABRT)\n", id);
        fflush(stdout);
        abort();                      /* Terminates the process with SIGABRT */
    }

    printf("  [Worker %d] finished normally\n", id);
    fflush(stdout);
    _exit(0);
}

void worker_report_status(int id, int pid, int status)
{
    if (WIFEXITED(status))
    {
        int code = WEXITSTATUS(status);
        if (code == 0)
        {
            printf("Worker %d (pid %d): completed NORMALLY\n", id, pid);
        }
        else
        {
            printf("Worker %d (pid %d): FAILED, exit code %d\n", id, pid, code);
        }
    }
    else if (WIFSIGNALED(status))
    {
        printf("Worker %d (pid %d): FAILED, killed by signal %d (%s)\n",
               id, pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
    }
}

void worker_demo(void)
{
    WorkerMode modes[NUM_WORKERS] =
    {
        WORKER_NORMAL, WORKER_NORMAL, WORKER_EXIT_ERROR, WORKER_CRASH
    };
    pid_t pids[NUM_WORKERS];
    int   i;

    printf("\n===== FR6: Worker Failure Detection =====\n");
    fflush(stdout);                   /* Avoid duplicated buffered output after fork */

    /* Spawn all workers */
    for (i = 0; i < NUM_WORKERS; i++)
    {
        pids[i] = fork();
        if (pids[i] < 0)
        {
            perror("fork");
            return;
        }
        if (pids[i] == 0)
        {
            worker_run(i, modes[i]);  /* Never returns */
        }
    }

    /* Parent waits for each worker and inspects how it ended */
    for (i = 0; i < NUM_WORKERS; i++)
    {
        int status;
        if (waitpid(pids[i], &status, 0) < 0)
        {
            perror("waitpid");
            continue;
        }
        worker_report_status(i, (int)pids[i], status);
    }
}
