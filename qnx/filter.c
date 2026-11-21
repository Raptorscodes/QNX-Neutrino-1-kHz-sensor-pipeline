/*
 * Stage 5 — filter process (start LOGGER first, then this, then sensor).
 *
 * Stage 4 flow (keep this):
 *   name_attach(FILTER_ATTACH_NAME)
 *   MsgReceive -> LPF -> MsgReply(sensor)
 *
 * Stage 5 addition:
 *   After MsgReply, MsgSend a log_record_t to the logger.
 *   Sensor is already unblocked; logger only copies to RAM + replies.
 *   Do NOT fwrite / printf in this hot path.
 *
 * Docs (QNX 8.0):
 *   name_attach / name_detach / name_open / name_close
 *   MsgReceive / MsgReply / MsgSend
 *
 * Fill in the Stage 5 TODOs. Your Stage 4 logic should stay.
 */

#define _QNX_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>

#include "protocol.h"

typedef union {
    struct _pulse   pulse;
    imu_sample_t    sample;
} filter_msg_t;

int main(void)
{
    name_attach_t *attach = NULL;
    filter_msg_t msg;
    filter_reply_t reply;
    rcvid_t rcvid;
    uint32_t seq = 0;
    float accel_z = 0.0f;
    float filt_z = 0.0f;
    const float alpha = 0.1f;

    int logger_coid = -1;
    log_record_t log_rec;
    log_reply_t log_reply;

    attach = name_attach(NULL, FILTER_ATTACH_NAME, 0);
    if (attach == NULL) {
        perror("name_attach");
        exit(EXIT_FAILURE);
    }
    int chid = attach->chid;

    logger_coid = name_open(LOGGER_ATTACH_NAME, 0);
    if (logger_coid < 0) {
        perror("name_open logger");
        exit(EXIT_FAILURE);
    }

    printf("filter: listening as \"%s\"\n", FILTER_ATTACH_NAME);
    fflush(stdout);

    for (;;) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid < 0) {
            perror("MsgReceive");
            exit(EXIT_FAILURE);
        } else if (rcvid > 0) {
            seq = msg.sample.seq;
            accel_z = msg.sample.accel_z;
            filt_z = alpha * accel_z + (1.0f - alpha) * filt_z;

            reply.seq = seq;
            reply.filtered_z = filt_z;
            reply.status = 0;

            if (MsgReply(rcvid, 0, &reply, sizeof(reply)) == -1) {
                perror("MsgReply");
                exit(EXIT_FAILURE);
            }

            log_rec.seq = seq;
            log_rec.timestamp_ns = msg.sample.timestamp_ns;
            log_rec.accel_z = accel_z;
            log_rec.filtered_z = filt_z;
            if (MsgSend(logger_coid, &log_rec, sizeof(log_rec),
                        &log_reply, sizeof(log_reply)) == -1) {
                perror("MsgSend logger");
                exit(EXIT_FAILURE);
            }

            if (seq == PIPELINE_SAMPLES - 1) {
                break;
            }
        }
    }

    name_close(logger_coid);

    name_detach(attach, 0);
    return EXIT_SUCCESS;
}
