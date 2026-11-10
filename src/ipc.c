#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "ipc.h"

int ipc_create(IpcContext *ctx)
{
    /* Remove leftovers from a previous crashed run */
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_READY_NAME);

    /* Create and size the shared memory object */
    ctx->shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (ctx->shmFd < 0)
    {
        perror("shm_open");
        return -1;
    }
    if (ftruncate(ctx->shmFd, sizeof(SharedData)) < 0)
    {
        perror("ftruncate");
        return -1;
    }

    /* Map it into this process's address space */
    ctx->data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE,
                     MAP_SHARED, ctx->shmFd, 0);
    if (ctx->data == MAP_FAILED)
    {
        perror("mmap");
        return -1;
    }
    memset(ctx->data, 0, sizeof(SharedData));

    /* mutex starts at 1 (unlocked), ready starts at 0 (nothing to read) */
    ctx->mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, 0600, 1);
    ctx->ready = sem_open(SEM_READY_NAME, O_CREAT, 0600, 0);
    if (ctx->mutex == SEM_FAILED || ctx->ready == SEM_FAILED)
    {
        perror("sem_open");
        return -1;
    }
    return 0;
}

int ipc_attach(IpcContext *ctx)
{
    /* Open the objects that the creator already made */
    ctx->shmFd = shm_open(SHM_NAME, O_RDWR, 0600);
    if (ctx->shmFd < 0)
    {
        perror("shm_open (attach)");
        return -1;
    }

    ctx->data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE,
                     MAP_SHARED, ctx->shmFd, 0);
    if (ctx->data == MAP_FAILED)
    {
        perror("mmap (attach)");
        return -1;
    }

    ctx->mutex = sem_open(SEM_MUTEX_NAME, 0);
    ctx->ready = sem_open(SEM_READY_NAME, 0);
    if (ctx->mutex == SEM_FAILED || ctx->ready == SEM_FAILED)
    {
        perror("sem_open (attach)");
        return -1;
    }
    return 0;
}

void ipc_write(IpcContext *ctx, const char *msg)
{
    sem_wait(ctx->mutex);                       /* Enter critical section */
    ctx->data->counter++;
    snprintf(ctx->data->message, MSG_SIZE, "%s", msg);
    sem_post(ctx->mutex);                       /* Leave critical section */

    sem_post(ctx->ready);                       /* Tell the reader data is ready */
}

void ipc_read(IpcContext *ctx, char *out, int outSize)
{
    sem_wait(ctx->ready);                       /* Block until a writer signals */

    sem_wait(ctx->mutex);
    snprintf(out, (size_t)outSize, "%s", ctx->data->message);
    sem_post(ctx->mutex);
}

void ipc_detach(IpcContext *ctx)
{
    sem_close(ctx->mutex);
    sem_close(ctx->ready);
    munmap(ctx->data, sizeof(SharedData));
    close(ctx->shmFd);
}

void ipc_destroy(IpcContext *ctx)
{
    ipc_detach(ctx);

    /* Unlink removes the names from the system (the actual cleanup) */
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_READY_NAME);
}

void ipc_demo(void)
{
    IpcContext parentCtx;
    char       buffer[MSG_SIZE];
    pid_t      pid;

    printf("\n===== FR4: Inter-process Communication =====\n");
    fflush(stdout);                             /* Avoid duplicated output after fork */

    if (ipc_create(&parentCtx) < 0)
    {
        return;
    }

    pid = fork();
    if (pid == 0)
    {
        /* Child = writer: attaches to the existing shm/semaphores */
        IpcContext childCtx;
        if (ipc_attach(&childCtx) == 0)
        {
            ipc_write(&childCtx, "Hello from child process!");
            ipc_detach(&childCtx);
        }
        _exit(0);
    }

    /* Parent = reader: blocks until the child signals */
    ipc_read(&parentCtx, buffer, (int)sizeof(buffer));
    printf("Parent received: \"%s\" (counter = %d)\n",
           buffer, parentCtx.data->counter);

    waitpid(pid, NULL, 0);
    ipc_destroy(&parentCtx);
    printf("IPC resources cleaned up.\n");
}
