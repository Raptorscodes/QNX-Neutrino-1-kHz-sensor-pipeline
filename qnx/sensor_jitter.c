/*
 * Stage 3 — measure 1 ms timer jitter; dump CSV after the run.
 *
 * Reuse Stage 2 setup (channel, ConnectAttach, pulse timer at 1 ms).
 * New ideas:+
 *   jitter(n) = t_actual(n) - (t0 + n * 1_000_000 ns)
 *   Preallocate an array; NO printf/fwrite inside the 1 ms path.
 *   clock_gettime(CLOCK_MONOTONIC) as the first action on each pulse.
 *   Optional: mlockall(MCL_CURRENT | MCL_FUTURE) at startup.
 *
 * Docs: same as Stage 2, plus clock_gettime, mlockall (sys/mman.h).
 *
 * Fill in the TODOs. Keep timer_tick.c as your Stage 2 reference.
 */

#define _QNX_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <time.h>

#define PULSE_CODE_TIMER  (_PULSE_CODE_MINAVAIL + 0)

/* 10 s at 1 kHz while learning; later bump toward 300000 (5 min). */
#define NUM_SAMPLES  10000

#define PERIOD_NS    1000000L   /* 1 ms */

typedef union {
    struct _pulse pulse;
} message_t;

/* Convert timespec to a single 64-bit nanosecond count. */
static int64_t timespec_to_ns(const struct timespec *ts)
{
    return ts->tv_sec * 1000000000LL + ts->tv_nsec;
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

int main(void)
{
    int chid;
    int coid;
    timer_t timerid;
    struct sigevent event;
    struct itimerspec itime;
    message_t msg;
    rcvid_t rcvid;

    /* Preallocated — do not malloc inside the loop. */
    static int64_t jitter_ns[NUM_SAMPLES];

    int64_t t0_ns = 0;
    int have_t0 = 0;
    size_t n = 0;


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

        rcvid = MsgReceive (chid, &msg, sizeof (msg), NULL);

        if(rcvid > 0){
            fprintf (stderr, "%s:  unexpected message, rcvid %ld\n", "timer_tick", (long)rcvid);
            perror (NULL);
            exit(EXIT_FAILURE);
        }
        else if(rcvid == -1){
            perror("MsgReceive");
            exit(EXIT_FAILURE);
        }

        else{
            if(msg.pulse.code == PULSE_CODE_TIMER){
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                int64_t now_ns = timespec_to_ns(&now);
                if(!have_t0){
                    t0_ns = now_ns;
                    have_t0 = 1;
                    jitter_ns[0] = 0;
                    n++;
                }
                else{
                    int64_t expected = t0_ns + (int64_t)n * PERIOD_NS;
                    jitter_ns[n] = now_ns - expected;
                    n++;
                }
                if(n >= NUM_SAMPLES){
                    timer_delete(timerid);
                    break;
                }
            }
        }
    }

    if (write_jitter_csv("jitter_idle.csv", jitter_ns, n) != 0) {
        return EXIT_FAILURE;
    }
printf("wrote %zu samples to jitter_idle.csv\n", n);

    return EXIT_SUCCESS;
}
