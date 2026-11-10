#ifndef BANKERS_H
#define BANKERS_H

#define MAX_PROCESSES 5
#define MAX_RESOURCES 3

typedef struct
{
    int numProcesses;
    int numResources;
    int available[MAX_RESOURCES];
    int max[MAX_PROCESSES][MAX_RESOURCES];
    int allocation[MAX_PROCESSES][MAX_RESOURCES];
    int need[MAX_PROCESSES][MAX_RESOURCES];      /* need = max - allocation */
} BankerState;

/* Return codes for bankers_request() */
typedef enum
{
    REQ_GRANTED       =  1,   /* Request granted, state stays safe */
    REQ_DENIED_UNSAFE =  0,   /* Would lead to unsafe state, rolled back */
    REQ_EXCEEDS_NEED  = -1,   /* Process asked for more than its declared max */
    REQ_MUST_WAIT     = -2    /* Not enough resources available right now */
} RequestResult;

void bankers_compute_need(BankerState *s);
int  bankers_is_safe(const BankerState *s, int safeSeq[]);
int  bankers_request(BankerState *s, int pid, const int request[]);
void bankers_print(const BankerState *s);
void bankers_demo(void);

#endif
