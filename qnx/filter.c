/*
  filter: MsgReceive sample → LPF → MsgReply sensor → MsgSend logger.
 * Start logger first, then this, then sensor.
 */

#define _QNX_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>

#include "protocol.h"

#define LPF_ALPHA  0.1f

typedef union {
    struct _pulse pulse;
    imu_sample_t  sample;
} filter_msg_t;

static float lpf_update(float *state, float sample, float alpha)
{
    *state = alpha * sample + (1.0f - alpha) * (*state);
    return *state;
}

static void handle_sample(rcvid_t rcvid,
                          const imu_sample_t *sample,
                          float *filt_z,
                          int logger_coid)
{
    filter_reply_t reply;
    log_record_t log_rec;
    log_reply_t log_reply;
    float filtered;

    filtered = lpf_update(filt_z, sample->accel_z, LPF_ALPHA);

    reply.seq = sample->seq;
    reply.filtered_z = filtered;
    reply.status = 0;
    if (MsgReply(rcvid, 0, &reply, sizeof(reply)) == -1) {
        perror("MsgReply");
        exit(EXIT_FAILURE);
    }

    log_rec.seq = sample->seq;
    log_rec.timestamp_ns = sample->timestamp_ns;
    log_rec.accel_z = sample->accel_z;
    log_rec.filtered_z = filtered;
    if (MsgSend(logger_coid, &log_rec, sizeof(log_rec),
                &log_reply, sizeof(log_reply)) == -1) {
        perror("MsgSend logger");
        exit(EXIT_FAILURE);
    }
}

static void run_filter_loop(int chid, int logger_coid)
{
    filter_msg_t msg;
    rcvid_t rcvid;
    float filt_z = 0.0f;

    for (;;) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid < 0) {
            perror("MsgReceive");
            exit(EXIT_FAILURE);
        }
        if (rcvid == 0) {
            continue; /* pulse */
        }

        handle_sample(rcvid, &msg.sample, &filt_z, logger_coid);

        if (msg.sample.seq == PIPELINE_SAMPLES - 1) {
            break;
        }
    }
}

int main(void)
{
    name_attach_t *attach;
    int logger_coid;

    attach = name_attach(NULL, FILTER_ATTACH_NAME, 0);
    if (attach == NULL) {
        perror("name_attach");
        exit(EXIT_FAILURE);
    }

    logger_coid = name_open(LOGGER_ATTACH_NAME, 0);
    if (logger_coid < 0) {
        perror("name_open logger");
        name_detach(attach, 0);
        exit(EXIT_FAILURE);
    }

    printf("filter: listening as \"%s\"\n", FILTER_ATTACH_NAME);
    fflush(stdout);

    run_filter_loop(attach->chid, logger_coid);

    name_close(logger_coid);
    name_detach(attach, 0);
    return EXIT_SUCCESS;
}
