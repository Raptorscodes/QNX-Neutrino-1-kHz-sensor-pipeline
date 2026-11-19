/*
 * Stage 2 — 1 ms timer pulse, print once per second.
 *
 * Flow:
 *   ChannelCreate  -> inbox
 *   ConnectAttach  -> connection so the timer can send a pulse to us
 *   SIGEV_PULSE_INIT + timer_create(CLOCK_MONOTONIC) + timer_settime(1 ms)
 *   loop: MsgReceive; on our timer pulse, tick++; every 1000 ticks, printf
 *
 * Fill in the TODOs using the QNX 8.0 docs (ChannelCreate, ConnectAttach,
 * sigevent / SIGEV_PULSE_INIT, timer_create, timer_settime, MsgReceive, _pulse).
 */

/* Need QNX/POSIX extensions with -std=c11 (CLOCK_MONOTONIC, SIGEV_PULSE_*). */
#define _QNX_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/neutrino.h>
#include <time.h>

/* Safe user pulse codes are in [_PULSE_CODE_MINAVAIL, _PULSE_CODE_MAXAVAIL]. */
#define PULSE_CODE_TIMER  (_PULSE_CODE_MINAVAIL + 0)

/*
 * MsgReceive can deliver either a pulse or a real message into the same
 * buffer. A union keeps the pulse layout at the start of that buffer.
 */
typedef union {
    struct _pulse pulse;
} message_t;

int main(void)
{
    int chid;
    int coid;
    timer_t timerid;
    struct sigevent event;
    struct itimerspec itime;
    message_t msg;
    rcvid_t rcvid;
    unsigned long tick = 0;

    if((chid = ChannelCreate(0)) == -1){
        fprintf (stderr, "%s:  couldn't create channel!\n", "timer_tick");
        perror("NULL");
        exit(EXIT_FAILURE);
    }



    if((coid = ConnectAttach(0, 0, chid, 0, 0)) == -1){
        fprintf (stderr, "%s:  couldn't ConnectAttach to self!\n", "timer_tick");
        perror("NULL");
        exit(EXIT_FAILURE);
    }



    SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, PULSE_CODE_TIMER, 0);



    if (timer_create(CLOCK_MONOTONIC, &event, &timerid) == -1) {
        fprintf (stderr, "%s:  couldn't create a timer, errno %d\n",
                 "timer_tick", errno);
        perror (NULL);
        exit (EXIT_FAILURE);
    }

    itime.it_value.tv_sec = 0;
    itime.it_value.tv_nsec = 1000000;
    itime.it_interval.tv_sec = 0;
    itime.it_interval.tv_nsec = 1000000;

    if (timer_settime(timerid, 0, &itime, NULL) == -1){
        fprintf (stderr, "%s:  couldn't set timer, errno %d\n" 
        , "timer_tick", errno);
        perror (NULL);
        exit(EXIT_FAILURE);
    }


    for (;;) {
        /* TODO 6: MsgReceive(chid, &msg, sizeof(msg), NULL) -> rcvid */
        rcvid = MsgReceive (chid, &msg, sizeof (msg), NULL);

        if(rcvid == 0){
            if(msg.pulse.code == PULSE_CODE_TIMER){
                tick++;
                if(tick % 1000 == 0){
                    printf("%lu ticks\n", tick);
                }
            }
        } else if(rcvid > 0){
            fprintf (stderr, "%s:  unexpected message, rcvid %ld\n", "timer_tick", (long)rcvid);
            perror (NULL);
            exit(EXIT_FAILURE);
        }
        if(rcvid == -1){
            perror("MsgReceive");
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

