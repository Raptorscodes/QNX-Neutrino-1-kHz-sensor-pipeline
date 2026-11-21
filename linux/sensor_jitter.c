/*
 * Stage 7 — Linux 1 ms jitter measurement (compare to QNX Stage 3/6).
 *
 * Same math as qnx/sensor_jitter.c:
 *   jitter(n) = t_actual(n) - (t0 + n * 1_000_000 ns)
 *
 * Linux: clock_nanosleep(..., TIMER_ABSTIME, ...) + optional SCHED_FIFO.
 *
 * Build: make linux
 * Plot:  python3 analysis/plot_jitter.py results/linux_jitter_idle.csv -o ...
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define NUM_SAMPLES      10000
#define PERIOD_NS        1000000L   /* 1 ms */
#define JITTER_PRIORITY  50

static int64_t timespec_to_ns(const struct timespec *ts)
{
    return (int64_t)ts->tv_sec * 1000000000LL + (int64_t)ts->tv_nsec;
}

static void timespec_add_ns(struct timespec *ts, int64_t add_ns)
{
    int64_t nsec = (int64_t)ts->tv_nsec + add_ns;

    ts->tv_sec += nsec / 1000000000LL;
    nsec %= 1000000000LL;
    if (nsec < 0) {
        nsec += 1000000000LL;
        ts->tv_sec -= 1;
    }
    ts->tv_nsec = (long)nsec;
}

static int write_jitter_csv(const char *path,
                            const int64_t *jitter_ns,
                            size_t count)
{
    FILE *fp;
    size_t i;

    fp = fopen(path, "w");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }

    if (fprintf(fp, "n,jitter_ns\n") < 0) {
        perror("fprintf");
        fclose(fp);
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (fprintf(fp, "%zu,%" PRId64 "\n", i, jitter_ns[i]) < 0) {
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

int main(int argc, char **argv)
{
    const char *csv_path = "linux_jitter_idle.csv";
    static int64_t jitter_ns[NUM_SAMPLES];
    struct timespec next;
    struct timespec now;
    int64_t t0_ns;
    size_t n;
    struct sched_param param;
    int rc;

    if (argc >= 2) {
        csv_path = argv[1];
    }

    memset(&param, 0, sizeof(param));
    param.sched_priority = JITTER_PRIORITY;
    rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (rc != 0) {
        errno = rc;
        perror("pthread_setschedparam (continuing at normal priority; try sudo)");
    } else {
        printf("linux jitter: SCHED_FIFO priority %d\n", JITTER_PRIORITY);
        fflush(stdout);
    }

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

    if (write_jitter_csv(csv_path, jitter_ns, NUM_SAMPLES) != 0) {
        return EXIT_FAILURE;
    }

    printf("wrote %zu samples to %s\n", (size_t)NUM_SAMPLES, csv_path);
    fflush(stdout);
    return EXIT_SUCCESS;
}
