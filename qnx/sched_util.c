#define _QNX_SOURCE

#include "sched_util.h"

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <sys/neutrino.h>

int set_thread_priority(int priority)
{
    struct sched_param param;

    param.sched_priority = priority;
    if (SchedSet(0, 0, SCHED_RR, &param) == -1) {
        perror("SchedSet");
        return -1;
    }
    return 0;
}
