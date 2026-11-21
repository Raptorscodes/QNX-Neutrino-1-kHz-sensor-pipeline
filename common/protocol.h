/*
 * Shared message layout for the sensor pipeline.
 *
 * Stage 4: sensor <-> filter (imu_sample_t / filter_reply_t)
 * Stage 5: filter  -> logger (log_record_t)
 *
 * All processes #include this so MsgSend / MsgReceive use the same
 * struct sizes and the same channel names.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define FILTER_ATTACH_NAME  "imu_filter"
#define LOGGER_ATTACH_NAME  "imu_logger"

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

/*
 * Filter -> Logger (MsgSend payload).
 * Logger copies this into a RAM array and MsgReply's quickly — no disk
 * in that path. CSV is written once at the end (same idea as Stage 3).
 */
typedef struct {
    uint32_t seq;
    int64_t  timestamp_ns;
    float    accel_z;      /* raw */
    float    filtered_z;   /* after LPF */
} log_record_t;

/*
 * Logger -> Filter (MsgReply payload). Keep it tiny.
 */
typedef struct {
    int32_t status;        /* 0 = ok, non-zero = drop / error */
} log_reply_t;

#endif
