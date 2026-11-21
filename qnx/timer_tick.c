/*
 * 1 ms timer pulse; print once per second (every 1000 ticks).
 */

#define _QNX_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>

#include "pulse_timer.h"

#define PULSE_CODE_TIMER  (_PULSE_CODE_MINAVAIL + 0)
#define PERIOD_NS         1000000L
#define TICKS_PER_PRINT   1000UL

typedef union {
    struct _pulse pulse;
} message_t;

static void run_tick_loop(int chid)
{
    message_t msg;
    rcvid_t rcvid;
    unsigned long tick = 0;

    for (;;) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid == -1) {
            perror("MsgReceive");
            exit(EXIT_FAILURE);
        }
        if (rcvid > 0) {
            fprintf(stderr, "timer_tick: unexpected message (rcvid=%ld)\n",
                    (long)rcvid);
            exit(EXIT_FAILURE);
        }

        if (msg.pulse.code != PULSE_CODE_TIMER) {
            continue;
        }

        tick++;
        if (tick % TICKS_PER_PRINT == 0) {
            printf("%lu ticks\n", tick);
            fflush(stdout);
        }
    }
}

int main(void)
{
    int chid;
    int coid;
    timer_t timerid;

    if (pulse_timer_create(PULSE_CODE_TIMER, PERIOD_NS,
                           &chid, &coid, &timerid) != 0) {
        exit(EXIT_FAILURE);
    }

    run_tick_loop(chid);
    return EXIT_SUCCESS;
}
