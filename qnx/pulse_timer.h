#ifndef PULSE_TIMER_H
#define PULSE_TIMER_H

#include <time.h>

/*
 * Create a self-connected channel and a periodic CLOCK_MONOTONIC pulse timer.
 *
 * On success: *chid_out, *coid_out, *timerid_out are set; returns 0.
 * On failure: prints an error and returns -1 
 */
int pulse_timer_create(int pulse_code,
                       long period_ns,
                       int *chid_out,
                       int *coid_out,
                       timer_t *timerid_out);

#endif
