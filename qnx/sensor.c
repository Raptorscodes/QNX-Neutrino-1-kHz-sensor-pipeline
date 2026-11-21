/*
 * sensor: 1 ms pulses → fake IMU sample → MsgSend to filter.
 * Start logger, then filter, then this.
 */

#define _QNX_SOURCE

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <time.h>

#include "protocol.h"
#include "pulse_timer.h"
#include "time_util.h"

#define PULSE_CODE_TIMER  (_PULSE_CODE_MINAVAIL + 0)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef union {
    struct _pulse pulse;
} timer_msg_t;

static float fake_accel_z(uint32_t seq)
{
    float t = (float)seq * 0.001f;
    float noise = (float)((int)(seq % 17) - 8) * 0.001f;
    return sinf(2.0f * (float)M_PI * 2.0f * t) + noise;
}

static void send_one_sample(int filter_coid, uint32_t seq)
{
    imu_sample_t sample;
    filter_reply_t reply;
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    sample.seq = seq;
    sample.timestamp_ns = timespec_to_ns(&now);
    sample.accel_z = fake_accel_z(seq);

    if (MsgSend(filter_coid, &sample, sizeof(sample),
                &reply, sizeof(reply)) == -1) {
        perror("MsgSend");
        exit(EXIT_FAILURE);
    }
    if (reply.status != 0) {
        fprintf(stderr, "filter returned status %d\n", reply.status);
        exit(EXIT_FAILURE);
    }
}

static void run_sensor_loop(int chid, int filter_coid, timer_t timerid)
{
    timer_msg_t tmsg;
    rcvid_t rcvid;
    uint32_t seq = 0;

    for (;;) {
        rcvid = MsgReceive(chid, &tmsg, sizeof(tmsg), NULL);
        if (rcvid == -1) {
            perror("MsgReceive");
            exit(EXIT_FAILURE);
        }
        if (rcvid != 0) {
            continue;
        }
        if (tmsg.pulse.code != PULSE_CODE_TIMER) {
            continue;
        }

        send_one_sample(filter_coid, seq);
        seq++;

        if (seq >= PIPELINE_SAMPLES) {
            timer_delete(timerid);
            printf("sent %u samples\n", seq);
            fflush(stdout);
            break;
        }
    }
}

int main(void)
{
    int chid;
    int coid;
    int filter_coid;
    timer_t timerid;

    if (pulse_timer_create(PULSE_CODE_TIMER, PERIOD_NS,
                           &chid, &coid, &timerid) != 0) {
        exit(EXIT_FAILURE);
    }

    filter_coid = name_open(FILTER_ATTACH_NAME, 0);
    if (filter_coid < 0) {
        perror("name_open");
        exit(EXIT_FAILURE);
    }

    printf("sensor: opened filter \"%s\"\n", FILTER_ATTACH_NAME);
    fflush(stdout);

    run_sensor_loop(chid, filter_coid, timerid);

    name_close(filter_coid);
    return EXIT_SUCCESS;
}
