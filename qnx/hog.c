/*
 * Low-priority CPU busy loop for load testing.
 */

#define _QNX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "sched_util.h"

#define HOG_PRIORITY  10

static void burn_cpu(void)
{
    volatile unsigned long sink = 0;

    for (;;) {
        sink++;
        if (sink == 0) {
            continue;
        }
    }
}

int main(void)
{
    if (set_thread_priority(HOG_PRIORITY) != 0) {
        exit(EXIT_FAILURE);
    }

    printf("hog: pid=%d priority=%d\n", (int)getpid(), HOG_PRIORITY);
    printf("hog: burning CPU (pid=%d)\n", (int)getpid());
    fflush(stdout);

    burn_cpu();
    return EXIT_SUCCESS;
