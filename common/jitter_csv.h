#ifndef JITTER_CSV_H
#define JITTER_CSV_H

#include <stddef.h>
#include <stdint.h>

/* Write header n,jitter_ns and one row per sample. Returns 0 on success. */
int write_jitter_csv(const char *path, const int64_t *jitter_ns, size_t count);

#endif
