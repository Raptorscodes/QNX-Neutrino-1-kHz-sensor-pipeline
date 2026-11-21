/*
 * Shared message layout for the sensor pipeline.
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
    uint32_t seq;      
    int64_t  timestamp_ns;
    float    accel_z;
} imu_sample_t;

/*
 * Filter -> Sensor (MsgReply payload).
 */
typedef struct {
    uint32_t seq;  
    float    filtered_z;
    int32_t  status;
} filter_reply_t;

/*
 * Filter -> Logger (MsgSend payload).
 */
typedef struct {
    uint32_t seq;
    int64_t  timestamp_ns;
    float    accel_z;
    float    filtered_z;
} log_record_t;

/*
 * Logger -> Filter (MsgReply payload).
 */
typedef struct {
    int32_t status;
} log_reply_t;

#endif
