#pragma once

#include "../defs.h"

#define completed_reads 0
#define reads_merged 1
#define sectors_read 2
#define reading_time_ms 3
#define completed_writes 4
#define writes_merged 5
#define sectors_written 6
#define writing_time_ms 7
#define io_in_flight 8
#define time_disk_busy_ms 9
#define weighted_time_in_queue_ms 10
#define discards_completed 11
#define discards_merged 12
#define discards_sectors 13
#define discards_time_ms 14

#define NUM_STAT_FIELDS 15
#define MIN_STAT_FIELDS 11

typedef struct {
    __attribute__((aligned(64))) uint64_t data[NUM_STAT_FIELDS];
    uint64_t count;
} io_stats;

typedef struct {
    char *data[NUM_STAT_FIELDS];
} io_stats_fmt;

bool io_statistics(const char *disk_name);
