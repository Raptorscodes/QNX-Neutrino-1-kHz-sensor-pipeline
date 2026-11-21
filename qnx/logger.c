/*
 * Stage 5 — logger process (start this FIRST, then filter, then sensor).
 *
 * Job:
 *   Receive log_record_t from the filter, store in a preallocated array,
 *   MsgReply immediately. After PIPELINE_SAMPLES records, write a CSV
 *   (disk I/O only at the end — never in the filter's hot path).
 *
 * Why a third process?
 *   Sensor/filter stay short. fwrite/printf belong here, after (or
 *   outside) the 1 ms chain. Filter does: MsgReply(sensor) THEN
 *   MsgSend(logger). Sensor is already free; logger only copies to RAM
 *   before replying.
 *
 * Docs (QNX 8.0):
 *   name_attach / name_detach
 *   MsgReceive / MsgReply
 *
 * Fill in the TODOs. Do not add CPU hogs yet (later stage).
 */

#define _QNX_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>

#include "protocol.h"

#define DEFAULT_CSV_PATH  "pipeline.csv"

typedef union {
    struct _pulse   pulse;
    log_record_t    record;
} logger_msg_t;

static int write_pipeline_csv(const char *path,
                              const log_record_t *records,
                              size_t count)
{
    FILE *fp;
    size_t i;

    fp = fopen(path, "w");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }

    if (fprintf(fp, "seq,timestamp_ns,accel_z,filtered_z\n") < 0) {
        perror("fprintf");
        fclose(fp);
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (fprintf(fp, "%" PRIu32 ",%" PRId64 ",%f,%f\n",
                    records[i].seq,
                    records[i].timestamp_ns,
                    (double)records[i].accel_z,
                    (double)records[i].filtered_z) < 0) {
            perror("fprintf");
            fclose(fp);
            return -1;
        }
    }

    if (fclose(fp) != 0) {
        perror("fclose");
        return -1;
    }

    return 0;
}

int main(void)
{
    name_attach_t *attach = NULL;
    logger_msg_t msg;
    log_reply_t reply;
    rcvid_t rcvid;
    size_t n = 0;


    static log_record_t records[PIPELINE_SAMPLES];


    attach = name_attach(NULL, LOGGER_ATTACH_NAME, 0);
    if (attach == NULL) {
        perror("name_attach");
        exit(EXIT_FAILURE);
    }

    int chid = attach->chid;

    printf("logger: listening as \"%s\"\n", LOGGER_ATTACH_NAME);
    fflush(stdout);

    for (;;) {
         rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
         if(rcvid < 0){
            perror("MsgReceive");
            exit(EXIT_FAILURE);
         }
         else if(rcvid > 0){
            if(n >= PIPELINE_SAMPLES){
                reply.status = -1;
                MsgReply(rcvid, 0, &reply, sizeof(reply));
                continue;
            }
            else{
                records[n] = msg.record;
                reply.status = 0;
                MsgReply(rcvid, 0, &reply, sizeof(reply));
                n++;
            }
            if(n == PIPELINE_SAMPLES){
                break;
            }
         }
    }


    name_detach(attach, 0);

    printf("logger: writing %zu records to %s\n", n, DEFAULT_CSV_PATH);
    fflush(stdout);

    if (write_pipeline_csv(DEFAULT_CSV_PATH, records, n) != 0) {
        fprintf(stderr, "logger: failed to write %s\n", DEFAULT_CSV_PATH);
        exit(EXIT_FAILURE);
    }

    printf("logger: done\n");
    fflush(stdout);
    return EXIT_SUCCESS;
}
