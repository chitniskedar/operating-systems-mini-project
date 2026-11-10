#include <stdio.h>
#include <string.h>
#include "bankers.h"

void bankers_compute_need(BankerState *s)
{
    int i;
    int j;

    for (i = 0; i < s->numProcesses; i++)
    {
        for (j = 0; j < s->numResources; j++)
        {
            s->need[i][j] = s->max[i][j] - s->allocation[i][j];
        }
    }
}

/* Returns 1 if process p's remaining need fits inside 'work' */
static int need_fits(const BankerState *s, int p, const int work[])
{
    int j;

    for (j = 0; j < s->numResources; j++)
    {
        if (s->need[p][j] > work[j])
        {
            return 0;
        }
    }
    return 1;
}

int bankers_is_safe(const BankerState *s, int safeSeq[])
{
    int work[MAX_RESOURCES];
    int finish[MAX_PROCESSES];
    int count = 0;
    int i;
    int j;

    /* work = available, finish = all false */
    for (j = 0; j < s->numResources; j++)
    {
        work[j] = s->available[j];
    }
    for (i = 0; i < s->numProcesses; i++)
    {
        finish[i] = 0;
    }

    while (count < s->numProcesses)
    {
        int found = 0;

        for (i = 0; i < s->numProcesses; i++)
        {
            if (!finish[i] && need_fits(s, i, work))
            {
                /* Pretend process i runs to completion and releases everything */
                for (j = 0; j < s->numResources; j++)
                {
                    work[j] += s->allocation[i][j];
                }
                finish[i] = 1;
                safeSeq[count++] = i;
                found = 1;
            }
        }

        if (!found)
        {
            return 0;    /* No process can proceed -> unsafe */
        }
    }
    return 1;            /* Everyone can finish -> safe */
}

int bankers_request(BankerState *s, int pid, const int request[])
{
    int j;
    int dummySeq[MAX_PROCESSES];

    /* Step 1: request must not exceed declared remaining need */
    for (j = 0; j < s->numResources; j++)
    {
        if (request[j] > s->need[pid][j])
        {
            return REQ_EXCEEDS_NEED;
        }
    }

    /* Step 2: request must fit in what is available right now */
    for (j = 0; j < s->numResources; j++)
    {
        if (request[j] > s->available[j])
        {
            return REQ_MUST_WAIT;
        }
    }

    /* Step 3: tentatively allocate */
    for (j = 0; j < s->numResources; j++)
    {
        s->available[j]       -= request[j];
        s->allocation[pid][j] += request[j];
        s->need[pid][j]       -= request[j];
    }

    /* Step 4: keep it only if the resulting state is safe */
    if (bankers_is_safe(s, dummySeq))
    {
        return REQ_GRANTED;
    }

    /* Roll back */
    for (j = 0; j < s->numResources; j++)
    {
        s->available[j]       += request[j];
        s->allocation[pid][j] -= request[j];
        s->need[pid][j]       += request[j];
    }
    return REQ_DENIED_UNSAFE;
}

void bankers_print(const BankerState *s)
{
    int i;
    int j;

    printf("Proc | Allocation | Max     | Need\n");
    for (i = 0; i < s->numProcesses; i++)
    {
        printf(" P%d  |", i);
        for (j = 0; j < s->numResources; j++) printf(" %d", s->allocation[i][j]);
        printf("      |");
        for (j = 0; j < s->numResources; j++) printf(" %d", s->max[i][j]);
        printf("   |");
        for (j = 0; j < s->numResources; j++) printf(" %d", s->need[i][j]);
        printf("\n");
    }
    printf("Available:");
    for (j = 0; j < s->numResources; j++) printf(" %d", s->available[j]);
    printf("\n");
}

/* Prints the safe sequence or "UNSAFE" for the given state */
static void check_and_print(const BankerState *s)
{
    int seq[MAX_PROCESSES];
    int i;

    if (bankers_is_safe(s, seq))
    {
        printf("Result: SAFE. Sequence: ");
        for (i = 0; i < s->numProcesses; i++)
        {
            printf("P%d%s", seq[i], (i < s->numProcesses - 1) ? " -> " : "\n");
        }
    }
    else
    {
        printf("Result: UNSAFE (no safe sequence exists)\n");
    }
}

void bankers_demo(void)
{
    /* Classic textbook example: 5 processes, 3 resource types (A, B, C) */
    BankerState s =
    {
        .numProcesses = 5,
        .numResources = 3,
        .available    = {3, 3, 2},
        .max          = {{7,5,3}, {3,2,2}, {9,0,2}, {2,2,2}, {4,3,3}},
        .allocation   = {{0,1,0}, {2,0,0}, {3,0,2}, {2,1,1}, {0,0,2}}
    };
    int reqGood[] = {1, 0, 2};
    int reqBad[]  = {0, 2, 0};
    int rc;

    printf("\n===== FR5: Banker's Algorithm =====\n");

    /* Test 1: initial state should be safe */
    printf("\n--- Test 1: initial state ---\n");
    bankers_compute_need(&s);
    bankers_print(&s);
    check_and_print(&s);

    /* Test 2: a request that keeps the system safe */
    printf("\n--- Test 2: P1 requests (1,0,2) ---\n");
    rc = bankers_request(&s, 1, reqGood);
    printf("Request %s\n", (rc == REQ_GRANTED) ? "GRANTED" : "DENIED");
    check_and_print(&s);

    /* Test 3: a request that would make the system unsafe */
    printf("\n--- Test 3: P0 requests (0,2,0) ---\n");
    rc = bankers_request(&s, 0, reqBad);
    printf("Request %s\n",
           (rc == REQ_DENIED_UNSAFE) ? "DENIED (would be unsafe)" : "other result");

    /* Test 4: a state that is unsafe from the start (nothing available) */
    printf("\n--- Test 4: unsafe state (Available = 0 0 0) ---\n");
    memset(s.available, 0, sizeof(s.available));
    bankers_compute_need(&s);
    check_and_print(&s);
}
