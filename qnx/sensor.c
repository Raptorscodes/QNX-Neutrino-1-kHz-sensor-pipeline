/*
 * Stage 4 — sensor process (start the FILTER first, then this).
 *
 * Combines:
 *   Stage 2/3: 1 ms timer pulse via ChannelCreate / ConnectAttach / timer_*
 *   Stage 4:   name_open(FILTER_ATTACH_NAME) + MsgSend each sample
 *
 * On each timer pulse:
 *   1) clock_gettime (optional: still store jitter like Stage 3)
 *   2) build imu_sample_t (seq, timestamp, fake accel_z)
 *   3) MsgSend to the filter; block until filter_reply_t comes back
 *
 * Docs (QNX 8.0):
 *   name_open / name_close
 *   MsgSend
 *   plus Stage 2 timer docs
 *
 * Stage 5: sensor is unchanged. Start logger, then filter, then this.
 */

#define _QNX_SOURCE

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/neutrino.h>
#include <time.h>
#include <sys/dispatch.h>



#include "protocol.h"

#define PULSE_CODE_TIMER  (_PULSE_CODE_MINAVAIL + 0)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef union {
    struct _pulse pulse;
} timer_msg_t;

static int64_t timespec_to_ns(const struct timespec *ts)
{
    return (int64_t)ts->tv_sec * 1000000000LL + ts->tv_nsec;
}

static float fake_accel_z(uint32_t seq)
{
    float t = (float)seq * 0.001f; /* 1 ms steps */
    float noise = (float)((int)(seq % 17) - 8) * 0.001f;
    return sinf(2.0f * (float)M_PI * 2.0f * t) + noise; /* ~2 Hz sine */
}

int main(void)
{
    int chid = -1;
    int coid = -1;          /* connection for the timer pulse */
    int filter_coid = -1;   /* connection from name_open */
    timer_t timerid;
    struct sigevent event;
    struct itimerspec itime;
    timer_msg_t tmsg;
    rcvid_t rcvid;

    imu_sample_t sample;
    filter_reply_t reply;

    uint32_t seq = 0;

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

    filter_coid = name_open(FILTER_ATTACH_NAME, 0);
    if (filter_coid < 0) {
        perror("name_open");
        exit(EXIT_FAILURE);
    }

    printf("sensor: opened filter \"%s\"\n", FILTER_ATTACH_NAME);
    fflush(stdout);

    for (;;) {

        rcvid = MsgReceive (chid, &tmsg, sizeof (tmsg), NULL);



        if(rcvid == 0){
            if(tmsg.pulse.code == PULSE_CODE_TIMER){
                sample.seq = seq;
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                sample.timestamp_ns = timespec_to_ns(&now);
                sample.accel_z = fake_accel_z(seq);
                rcvid = MsgSend(filter_coid, &sample, sizeof(sample), &reply, sizeof(reply));
                if(rcvid < 0){
                    perror("MsgSend");
                    exit(EXIT_FAILURE);
                }
                if(reply.status != 0){
                    fprintf(stderr, "filter returned status %d\n", reply.status);
                    exit(EXIT_FAILURE);
                }
                seq++;
                if(seq >= PIPELINE_SAMPLES){
                    timer_delete(timerid);
                    name_close(filter_coid);
                    printf("sent %u samples\n", seq);
                    fflush(stdout);
                    break;
                }
            }
        }

        else if(rcvid == -1){
            perror("MsgReceive");
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}
