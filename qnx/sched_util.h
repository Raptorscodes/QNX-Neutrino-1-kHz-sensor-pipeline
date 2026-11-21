#ifndef SCHED_UTIL_H
#define SCHED_UTIL_H

/* Set this thread's SCHED_RR priority. Returns 0 on success, -1 on error. */
int set_thread_priority(int priority);

#endif
