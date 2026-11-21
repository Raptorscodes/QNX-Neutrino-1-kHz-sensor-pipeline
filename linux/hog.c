/*
 * Stage 7 — Linux CPU hog (low-priority busy loop).
 *
 * Same job as qnx/hog.c: burn a core so linux/sensor_jitter can be
 * measured under load. No QNX APIs — plain POSIX/Linux.
 *
 * Run (on the Linux host), example with 4 hogs:
 *   ./build/linux/hog &
 *   ./build/linux/hog &
 *   ./build/linux/hog &
 *   ./build/linux/hog &
 *   ./build/linux/sensor_jitter results/linux_jitter_hogs.csv
 *   killall hog
 */

#define _DEFAULT_SOURCE  /* nice(3) with -std=c11 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    volatile unsigned long sink = 0;

    errno = 0;
    if (nice(10) == -1 && errno != 0) {
        perror("nice");
        /* non-fatal — still burn CPU at default nice */
    }

    printf("linux hog: burning CPU (pid=%d)\n", (int)getpid());
    fflush(stdout);

    for (;;) {
        sink++;
        if (sink == 0) {
            /* read sink so -Wunused-but-set-variable stays quiet */
            continue;
        }
    }

    return EXIT_SUCCESS;
}
