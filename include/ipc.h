#ifndef IPC_H
#define IPC_H

#include <semaphore.h>

#define SHM_NAME        "/os_proj_shm"     /* POSIX shared memory object name */
#define SEM_MUTEX_NAME  "/os_proj_mutex"   /* Named semaphore: mutual exclusion */
#define SEM_READY_NAME  "/os_proj_ready"   /* Named semaphore: "data is ready" signal */
#define MSG_SIZE        128

/* Layout of the data that lives inside shared memory */
typedef struct
{
    int  counter;              /* Number of messages written so far */
    char message[MSG_SIZE];    /* Latest message */
} SharedData;

/* Everything a process needs to use the IPC channel */
typedef struct
{
    int         shmFd;         /* File descriptor of the shm object */
    SharedData *data;          /* Mapped pointer to the shared memory */
    sem_t      *mutex;         /* Protects data */
    sem_t      *ready;         /* Signals the reader that data is available */
} IpcContext;

int  ipc_create(IpcContext *ctx);   /* Creator: makes shm + semaphores */
int  ipc_attach(IpcContext *ctx);   /* Other process: opens existing ones */
void ipc_write(IpcContext *ctx, const char *msg);
void ipc_read(IpcContext *ctx, char *out, int outSize);
void ipc_detach(IpcContext *ctx);   /* Unmap and close (does not delete) */
void ipc_destroy(IpcContext *ctx);  /* Detach + unlink names (cleanup) */
void ipc_demo(void);

#endif
