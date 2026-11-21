/*
 * Stage 6 — CPU hog (low priority busy loop).
 *
 * Purpose:
 *   Saturate a core with useless work at LOW priority so we can see
 *   whether a HIGH-priority 1 ms timer task (sensor_jitter) still wakes
 *   on time.
 *
 * Run (on the VM), example with 4 hogs:
 *   ./hog &
 *   ./hog &
 *   ./hog &
 *   ./hog &
 *   ./sensor_jitter          # writes CSV; use a new name for the loaded run
 *   slay hog                 # stop hogs when done
 *
 * Docs (QNX 8.0):
 *   SchedSet / SchedGet
 *   struct sched_param, SCHED_RR or SCHED_FIFO (sched.h)
 *
 * Fill in the TODOs. Do not add Linux comparison yet (Stage 7).
 */

#define _QNX_SOURCE

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <unistd.h>

/* Keep this BELOW the priority you set on sensor_jitter (Stage 6 TODO there). */
#define HOG_PRIORITY  10

int main(void)
{
    struct sched_param param;
    volatile unsigned long sink = 0;

    param.sched_priority = HOG_PRIORITY;
    if (SchedSet(0, 0, SCHED_RR, &param) == -1) {
        perror("SchedSet");
        exit(EXIT_FAILURE);
    }


    if (SchedGet(0, 0, &param) == -1) {
        perror("SchedGet");
        exit(EXIT_FAILURE);
    }
    printf("hog: pid=%d priority=%d\n", (int)getpid(), param.sched_priority);
    fflush(stdout);

    printf("hog: burning CPU (pid=%d)\n", (int)getpid());
    fflush(stdout);

    
     for(;;){
        sink++;
     }

     return EXIT_SUCCESS;
}
