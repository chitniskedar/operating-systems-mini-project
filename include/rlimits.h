#ifndef RLIMITS_H
#define RLIMITS_H

#include <sys/resource.h>

int  limits_set_soft(int resource, rlim_t value);   /* Wrapper around setrlimit */
void limits_print(const char *name, int resource);  /* Print current limit */
void limits_demo(void);

#endif
