/*
 * QNX sensor jitter measurement.
 */

#define _QNX_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <time.h>

#include "jitter_csv.h"
#include "pulse_timer.h"
#include "sched_util.h"
#include "time_util.h"

#define PULSE_CODE_TIMER  (_PULSE_CODE_MINAVAIL + 0)
#define NUM_SAMPLES       10000
#define PERIOD_NS         1000000L
#define JITTER_PRIORITY   50

typedef union {
    struct _pulse pulse;
} message_t;

static size_t collect_jitter(int chid, timer_t timerid, int64_t *jitter_ns)
{
    message_t msg;
    rcvid_t rcvid;
    int64_t t0_ns = 0;
    int have_t0 = 0;
    size_t n = 0;

    while (n < NUM_SAMPLES) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid == -1) {
            perror("MsgReceive");
            exit(EXIT_FAILURE);
        }
        if (rcvid > 0) {
            fprintf(stderr, "sensor_jitter: unexpected message (rcvid=%ld)\n",
                    (long)rcvid);
            exit(EXIT_FAILURE);
        }
        if (msg.pulse.code != PULSE_CODE_TIMER) {
            continue;
        }

        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            perror("clock_gettime");
            exit(EXIT_FAILURE);
        }

        int64_t now_ns = timespec_to_ns(&now);
        if (!have_t0) {
            t0_ns = now_ns;
            have_t0 = 1;
            jitter_ns[0] = 0;
        } else {
            int64_t expected = t0_ns + (int64_t)n * PERIOD_NS;
            jitter_ns[n] = now_ns - expected;
        }
        n++;
    }

    timer_delete(timerid);
    return n;
}

int main(int argc, char **argv)
{
    const char *csv_path = "jitter_idle.csv";
    static int64_t jitter_ns[NUM_SAMPLES];
    int chid;
    int coid;
    timer_t timerid;
    size_t n;

    if (argc >= 2) {
        csv_path = argv[1];
    }

    if (set_thread_priority(JITTER_PRIORITY) != 0) {
        exit(EXIT_FAILURE);
    }

    if (pulse_timer_create(PULSE_CODE_TIMER, PERIOD_NS,
                           &chid, &coid, &timerid) != 0) {
        exit(EXIT_FAILURE);
    }

    n = collect_jitter(chid, timerid, jitter_ns);

    if (write_jitter_csv(csv_path, jitter_ns, n) != 0) {
        return EXIT_FAILURE;
    }

    printf("wrote %zu samples to %s\n", n, csv_path);
    fflush(stdout);
    return EXIT_SUCCESS;
}
