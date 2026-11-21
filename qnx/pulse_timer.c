#define _QNX_SOURCE

#include "pulse_timer.h"

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/neutrino.h>

int pulse_timer_create(int pulse_code,
                       long period_ns,
                       int *chid_out,
                       int *coid_out,
                       timer_t *timerid_out)
{
    int chid;
    int coid;
    timer_t timerid;
    struct sigevent event;
    struct itimerspec itime;

    chid = ChannelCreate(0);
    if (chid == -1) {
        perror("ChannelCreate");
        return -1;
    }

    coid = ConnectAttach(0, 0, chid, 0, 0);
    if (coid == -1) {
        perror("ConnectAttach");
        ChannelDestroy(chid);
        return -1;
    }

    SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, pulse_code, 0);

    if (timer_create(CLOCK_MONOTONIC, &event, &timerid) == -1) {
        perror("timer_create");
        ConnectDetach(coid);
        ChannelDestroy(chid);
        return -1;
    }

    itime.it_value.tv_sec = 0;
    itime.it_value.tv_nsec = period_ns;
    itime.it_interval.tv_sec = 0;
    itime.it_interval.tv_nsec = period_ns;

    if (timer_settime(timerid, 0, &itime, NULL) == -1) {
        perror("timer_settime");
        timer_delete(timerid);
        ConnectDetach(coid);
        ChannelDestroy(chid);
        return -1;
    }

    *chid_out = chid;
    *coid_out = coid;
    *timerid_out = timerid;
    return 0;
}
