/*
 * Stage 4 — shared message layout between sensor and filter.
 *
 * Both processes #include this header so MsgSend / MsgReceive use the
 * same struct sizes and the same channel name.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define FILTER_ATTACH_NAME  "imu_filter"

#define PIPELINE_SAMPLES    10000

#define PERIOD_NS           1000000L  /* 1 ms */

/*
 * Sensor -> Filter (MsgSend payload).
 */
typedef struct {
    uint32_t seq;          /* 0, 1, 2, ... */
    int64_t  timestamp_ns; /* CLOCK_MONOTONIC when the sensor woke */
    float    accel_z;      /* fake reading: sine + noise */
} imu_sample_t;

/*
 * Filter -> Sensor (MsgReply payload).
 */
typedef struct {
    uint32_t seq;          /* echo sample seq */
    float    filtered_z;   /* complementary / simple LPF output */
    int32_t  status;       /* 0 = ok */
} filter_reply_t;

#endif
