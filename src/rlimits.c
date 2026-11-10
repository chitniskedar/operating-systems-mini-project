#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "rlimits.h"

int limits_set_soft(int resource, rlim_t value)
{
    struct rlimit rl;

    /* Read current limits so we keep the hard limit unchanged */
    if (getrlimit(resource, &rl) < 0)
    {
        perror("getrlimit");
        return -1;
    }

    rl.rlim_cur = value;              /* Only lower the soft limit */
    if (setrlimit(resource, &rl) < 0)
    {
        perror("setrlimit");
        return -1;
    }
    return 0;
}

void limits_print(const char *name, int resource)
{
    struct rlimit rl;

    if (getrlimit(resource, &rl) == 0)
    {
        printf("  %s: soft = %llu, hard = %llu\n", name,
               (unsigned long long)rl.rlim_cur,
               (unsigned long long)rl.rlim_max);
    }
}

/* ---------- Individual limit tests (each runs in its own child) ---------- */

static void test_memory_limit(void)
{
    size_t request = 256UL * 1024 * 1024;       /* 256 MB */
    void  *p;

    limits_set_soft(RLIMIT_AS, 64UL * 1024 * 1024);   /* Cap address space to 64 MB */
    limits_print("RLIMIT_AS", RLIMIT_AS);

    p = malloc(request);
    if (p == NULL)
    {
        printf("  malloc(256 MB) FAILED as expected: %s\n", strerror(errno));
    }
    else
    {
        printf("  malloc(256 MB) succeeded (limit not enforced?)\n");
        free(p);
    }
}

static void test_file_limit(void)
{
    int fds[64];
    int count = 0;

    limits_set_soft(RLIMIT_NOFILE, 10);         /* Allow only 10 open files */
    limits_print("RLIMIT_NOFILE", RLIMIT_NOFILE);

    /* Keep opening files until the limit is reached */
    while (count < 64)
    {
        fds[count] = open("/dev/null", O_RDONLY);
        if (fds[count] < 0)
        {
            printf("  open() failed after %d extra files: %s\n",
                   count, strerror(errno));
            break;
        }
        count++;
    }

    while (count > 0)
    {
        close(fds[--count]);
    }
}

static void test_cpu_limit(void)
{
    volatile unsigned long long x = 0;

    limits_set_soft(RLIMIT_CPU, 1);             /* 1 second of CPU time */
    limits_print("RLIMIT_CPU", RLIMIT_CPU);
    printf("  Entering busy loop; kernel should send SIGXCPU...\n");
    fflush(stdout);

    while (1)
    {
        x++;                                    /* Burn CPU */
    }
}

/* Forks, runs the given test in the child, and reports how it ended.
   Running in a child keeps the new limits from affecting the parent. */
static void run_in_child(const char *title, void (*test)(void))
{
    int   status;
    pid_t pid;

    printf("\n--- %s ---\n", title);
    fflush(stdout);

    pid = fork();
    if (pid == 0)
    {
        test();
        fflush(stdout);
        _exit(0);
    }

    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status))
    {
        printf("  Child terminated by signal %d (%s)\n",
               WTERMSIG(status), strsignal(WTERMSIG(status)));
    }
    else
    {
        printf("  Child exited with status %d\n", WEXITSTATUS(status));
    }
}

void limits_demo(void)
{
    printf("\n===== FR7: Resource Limits =====\n");

    run_in_child("Memory limit (RLIMIT_AS)",        test_memory_limit);
    run_in_child("Open-file limit (RLIMIT_NOFILE)", test_file_limit);
    run_in_child("CPU-time limit (RLIMIT_CPU)",     test_cpu_limit);
}
