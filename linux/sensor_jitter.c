/*
  Linux 1 ms jitter
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "jitter_csv.h"
#include "time_util.h"

#define NUM_SAMPLES      10000
#define PERIOD_NS        1000000L
#define JITTER_PRIORITY  50

static void try_set_fifo_priority(int priority)
{
    struct sched_param param;
    int rc;

    memset(&param, 0, sizeof(param));
    param.sched_priority = priority;
    rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (rc != 0) {
        errno = rc;
        perror("pthread_setschedparam (continuing at normal priority; try sudo)");
        return;
    }

    printf("linux jitter: SCHED_FIFO priority %d\n", priority);
    fflush(stdout);
}

static void collect_jitter(int64_t *jitter_ns)
{
    struct timespec next;
    struct timespec now;
    int64_t t0_ns;
    size_t n;
    int rc;

    if (clock_gettime(CLOCK_MONOTONIC, &next) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    t0_ns = timespec_to_ns(&next);

    for (n = 0; n < NUM_SAMPLES; n++) {
        do {
            rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        } while (rc == EINTR);

        if (rc != 0) {
            errno = rc;
            perror("clock_nanosleep");
            exit(EXIT_FAILURE);
        }

        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            perror("clock_gettime");
            exit(EXIT_FAILURE);
        }

        jitter_ns[n] = timespec_to_ns(&now) - (t0_ns + (int64_t)n * PERIOD_NS);
        timespec_add_ns(&next, PERIOD_NS);
    }
}

int main(int argc, char **argv)
{
    const char *csv_path = "linux_jitter_idle.csv";
    static int64_t jitter_ns[NUM_SAMPLES];

    if (argc >= 2) {
        csv_path = argv[1];
    }

    try_set_fifo_priority(JITTER_PRIORITY);
    collect_jitter(jitter_ns);

    if (write_jitter_csv(csv_path, jitter_ns, NUM_SAMPLES) != 0) {
        return EXIT_FAILURE;
    }

    printf("wrote %zu samples to %s\n", (size_t)NUM_SAMPLES, csv_path);
    fflush(stdout);
    return EXIT_SUCCESS;
}
