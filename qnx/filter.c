/*
 * Stage 4 — filter process (run this BEFORE the sensor).
 *
 * Flow:
 *   name_attach(FILTER_ATTACH_NAME)  -> creates a channel + advertises a name
 *   loop:
 *     MsgReceive  -> imu_sample_t
 *     run a SHORT filter (no malloc, no printf in the hot path)
 *     MsgReply    -> filter_reply_t
 *
 * Docs (QNX 8.0):
 *   name_attach / name_detach
 *   MsgReceive / MsgReply
 *   (same pulse notes as Stage 2 if you ever receive pulses on this channel)
 *
 * Fill in the TODOs. Do not add the logger yet (Stage 5).
 */

#define _QNX_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>

#include "protocol.h"

/*
 * Receive buffer: may get a pulse OR a real message. Union keeps _pulse
 * at the start (same idea as Stage 2/3).
 */
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
    const float alpha = 0.1f; /* TODO: tune later; keep deterministic */

    attach = name_attach(NULL, FILTER_ATTACH_NAME, 0);
    if (attach == NULL) {
        perror("name_attach");
        exit(EXIT_FAILURE);
    }
    int chid = attach->chid;
    printf("filter: listening as \"%s\"\n", FILTER_ATTACH_NAME);
    fflush(stdout);

    for (;;) {

        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid < 0) {
            perror("msgReceive");
            exit(EXIT_FAILURE);
        }

        else if(rcvid > 0){
            seq = msg.sample.seq;
            accel_z = msg.sample.accel_z;
            filt_z = alpha * accel_z + (1-alpha) * filt_z;
            reply.seq = seq;
            reply.filtered_z = filt_z;
            reply.status = 0;
            MsgReply(rcvid, 0, &reply, sizeof(reply));

            if(seq == PIPELINE_SAMPLES - 1){
                break;
            }
        }

     

    }

    name_detach(attach, 0);

    return EXIT_SUCCESS;
}
