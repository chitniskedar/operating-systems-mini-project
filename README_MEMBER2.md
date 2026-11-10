# Member 2: IPC and Resource Management (FR4 to FR7)

Part of the CampusCompute operating systems mini project.
This part covers inter-process communication, deadlock avoidance, worker failure
detection and resource limits. All code is C using POSIX APIs.

## Files

| File | Requirement | Purpose |
|---|---|---|
| `include/ipc.h`, `src/ipc.c` | FR4 | POSIX shared memory and named semaphores |
| `include/bankers.h`, `src/bankers.c` | FR5 | Banker's algorithm and safe sequence detection |
| `include/worker.h`, `src/worker.c` | FR6 | `fork()` and `waitpid()` worker failure detection |
| `include/rlimits.h`, `src/rlimits.c` | FR7 | `setrlimit()` resource limit demonstrations |

The resource limits header is named `rlimits.h` (not `limits.h`) so it does not
shadow the system `<limits.h>` when compiling with `-Iinclude`.

## FR4: Inter-process Communication
- Shared memory object `/os_proj_shm` created with `shm_open`, sized with `ftruncate`, mapped with `mmap`.
- Two named semaphores:
  - `/os_proj_mutex` (starts at 1) protects the shared data.
  - `/os_proj_ready` (starts at 0) tells the reader a message is available.
- Demo: the parent creates the channel and forks a child. The child attaches and writes a message, and the parent blocks until it receives it.
- Cleanup: `ipc_destroy()` calls `munmap`, `close`, `sem_close`, `shm_unlink` and `sem_unlink`.

## FR5: Banker's Algorithm
- Data per process: Available, Allocation, Maximum, Need (`Need = Max - Allocation`).
- `bankers_is_safe()` searches for a safe sequence.
- `bankers_request()` checks the request against Need and Available, allocates tentatively, and rolls back if the new state is unsafe.
- Demo uses the classic 5-process, 3-resource example:
  1. Initial state is safe (sequence P1 -> P3 -> P4 -> P0 -> P2).
  2. P1 requests (1,0,2): granted.
  3. P0 requests (0,2,0): denied, would be unsafe.
  4. Available set to 0 0 0: unsafe.

## FR6: Worker Failure Detection
- The parent forks 4 workers and calls `waitpid()` on each.
- Worker behaviours: 2 finish normally, 1 exits with code 3, 1 crashes with `SIGABRT`.
- `WIFEXITED` / `WEXITSTATUS` and `WIFSIGNALED` / `WTERMSIG` classify each outcome.

## FR7: Resource Limits
Each test runs in a forked child so the parent's limits are not affected.

| Limit | Test | Expected result |
|---|---|---|
| `RLIMIT_AS` (64 MB) | `malloc(256 MB)` | `malloc` returns NULL |
| `RLIMIT_NOFILE` (10) | open `/dev/null` repeatedly | `open()` fails with `EMFILE` |
| `RLIMIT_CPU` (1 s) | busy loop | child killed by `SIGXCPU` |

## Integrating with Member 1's code

**1. `src/main.c`**: add the includes:
```c
#include "ipc.h"
#include "bankers.h"
#include "worker.h"
#include "rlimits.h"
```
Extend the menu to `1..10` (Exit becomes 10) and add these branches before the final `else`:
```c
else if (choice == 6) { logger_log(&logger, "FR4 IPC demo started");       ipc_demo(); }
else if (choice == 7) { logger_log(&logger, "FR5 Banker's demo started");  bankers_demo(); }
else if (choice == 8) { logger_log(&logger, "FR6 worker demo started");    worker_demo(); }
else if (choice == 9) { logger_log(&logger, "FR7 limits demo started");    limits_demo(); }
```
Do not call `logger_log` from inside any forked child.

**2. `Makefile`**: add the sources and POSIX flags:
```makefile
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude
SOURCES += src/ipc.c src/bankers.c src/worker.c src/rlimits.c
LIBS    = -lrt          # Linux only
# link line: $(CC) $(CFLAGS) -pthread $(SOURCES) -o $(TARGET) $(LIBS)
```

## Build and run
```bash
make clean
make
./campuscompute
```
Menu options 6 to 9 run FR4 to FR7.

## Platform notes
- Requires Linux, WSL or macOS (uses `fork`, `shm_open`, `sem_open`, `setrlimit`). It does not build on native Windows.
- `RLIMIT_AS` is not enforced on macOS, so the memory test will report that `malloc` succeeded there.
- If a run is killed, stale named objects may remain in `/dev/shm`. They are removed automatically at the start of the next FR4 run.
