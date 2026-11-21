#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#include <stdint.h>
#include <time.h>

static inline int64_t timespec_to_ns(const struct timespec *ts)
{
    return (int64_t)ts->tv_sec * 1000000000LL + (int64_t)ts->tv_nsec;
}

static inline void timespec_add_ns(struct timespec *ts, int64_t add_ns)
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

#endif
