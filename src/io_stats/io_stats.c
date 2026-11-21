#include "io_stats.h"
#include "io_ui.h"
#include "../defs.h"
#include "../Debug/sectr_err.h"
#include <errno.h>
#include <inttypes.h>
#include "../sectr_helper/sectr_helper.h"

#define SECTOR_SIZE     512ULL
#define FMT_BUF_SIZE    64


typedef enum { UNIT_COUNT, UNIT_SECTORS, UNIT_MS } stat_unit;

/* Unit of each field, taken from the table */
static const stat_unit field_units[NUM_STAT_FIELDS] = {
    [completed_reads]           = UNIT_COUNT,
    [reads_merged]              = UNIT_COUNT,
    [sectors_read]              = UNIT_SECTORS,
    [reading_time_ms]           = UNIT_MS,
    [completed_writes]          = UNIT_COUNT,
    [writes_merged]             = UNIT_COUNT,
    [sectors_written]           = UNIT_SECTORS,
    [writing_time_ms]           = UNIT_MS,
    [io_in_flight]              = UNIT_COUNT,
    [time_disk_busy_ms]         = UNIT_MS,
    [weighted_time_in_queue_ms] = UNIT_MS,
    [discards_completed]        = UNIT_COUNT,
    [discards_merged]           = UNIT_COUNT,
    [discards_sectors]          = UNIT_SECTORS,
    [discards_time_ms]          = UNIT_MS,
    // [flush_completed]           = UNIT_COUNT,
    // [flush_time_ms]             = UNIT_MS,
};

static void format_bytes(uint64_t bytes, char *buf, size_t size) {
    if (bytes >= 1000000000ULL)
        snprintf(buf, size, "%.2f GB (%" PRIu64 " bytes)", (double)bytes / 1e9, bytes);
    else if (bytes >= 1000000ULL)
        snprintf(buf, size, "%.2f MB (%" PRIu64 " bytes)", (double)bytes / 1e6, bytes);
    else if (bytes >= 1000ULL)
        snprintf(buf, size, "%.2f kB (%" PRIu64 " bytes)", (double)bytes / 1e3, bytes);
    else
        snprintf(buf, size, "%" PRIu64 " bytes", bytes);
}

static void free_io_stats_fmt(io_stats_fmt *fmt_stats) {
    for (size_t i = 0; i < NUM_STAT_FIELDS; i++) {
        free(fmt_stats->data[i]);
        fmt_stats->data[i] = NULL;
    }
}

/*
 * Converts the raw values in stats in place:
 *   - sector fields become bytes (sectors * 512)
 *   - millisecond fields stay as ms in the integer array (shown as seconds in the strings)
 *   - count fields are unchanged
 * It also fills fmt_stats with a heap-allocated string per field.
 * Call this once per io_stats, since sectors are multiplied in place.
 * Free the strings with free_io_stats_fmt().
 */
static bool convert_stats_to_unit(io_stats *stats, io_stats_fmt *fmt_stats) {
    if (!stats || !fmt_stats) return false;

    memset(fmt_stats, 0, sizeof(*fmt_stats));

    for (size_t i = 0; i < NUM_STAT_FIELDS; i++) {
        char *buf = malloc(FMT_BUF_SIZE);
        if (!buf) {
            free_io_stats_fmt(fmt_stats);
            return false;
        }

        /* Field not reported by this kernel (e.g. discards on pre-4.18) */
        if (i >= stats->count) {
            snprintf(buf, FMT_BUF_SIZE, "n/a");
            fmt_stats->data[i] = buf;
            continue;
        }

        uint64_t raw = stats->data[i];

        switch (field_units[i]) {
        case UNIT_SECTORS:
            stats->data[i] = raw * SECTOR_SIZE;
            format_bytes(stats->data[i], buf, FMT_BUF_SIZE);
            break;

        case UNIT_MS:
            snprintf(buf, FMT_BUF_SIZE, "%.3f s (%" PRIu64 " ms)", (double)raw / 1000.0, raw);
            break;

        case UNIT_COUNT:
        default:
            snprintf(buf, FMT_BUF_SIZE, "%" PRIu64 " %s", raw,
                     i == io_in_flight ? " " : " ");
            break;
        }

        fmt_stats->data[i] = buf;
    }

    return true;
}


static io_stats *read_io_stats(const char *path) {
    io_stats *stats = malloc(sizeof(io_stats));
    if (!stats) {
        ERR(ERRC_AllocationFault, NULL, "Failed to allocate memory for I/O stats");
        return NULL;
    }
    memset(stats, 0, sizeof(io_stats));

    errno = 0;

    FILE *file = fopen(path, "r");
    if (!file) {
        ERR(ERRC_IOError, NULL, "Failed to open %s: %s", path, strerror(errno));
        free(stats);
        return NULL;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        const int32_t parsed = sscanf(line, " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                         " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                         " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                         " %" SCNu64 " %" SCNu64 " %" SCNu64,
                   &stats->data[completed_reads],
                   &stats->data[reads_merged],
                   &stats->data[sectors_read],
                   &stats->data[reading_time_ms],
                   &stats->data[completed_writes],
                   &stats->data[writes_merged],
                   &stats->data[sectors_written],
                   &stats->data[writing_time_ms],
                   &stats->data[io_in_flight],
                   &stats->data[time_disk_busy_ms],
                   &stats->data[weighted_time_in_queue_ms],
                   &stats->data[discards_completed],
                   &stats->data[discards_merged],
                   &stats->data[discards_sectors],
                   &stats->data[discards_time_ms]
                );
        if (parsed >= MIN_STAT_FIELDS) {
            stats->count = (uint64_t)parsed;
            break ;
        } else {
            ERR(ERRC_CorruptedData, NULL, "Unexpected format in %s (%d numbers)", path, parsed);
            fclose(file);
            free(stats);
            return NULL;
        }

    }

    fclose(file);

    if (stats->count == 0) {
        ERR(ERRC_CorruptedData, NULL, "%s is empty", path);
        free(stats);
        return NULL;
    }

    return stats;
}

bool io_statistics(const char *disk_name) {
    
    char *path_buffer = (char*)malloc(256);
    if (!path_buffer) {
        ERR(ERRC_AllocationFault, NULL, "Failed to allocate path_buffer");
        return false;
    }

    char basename[DEVICE_NAME_SIZE];
    if (!device_basename(disk_name, basename, sizeof(basename))) {
        ERR(ERRC_CorruptedData, NULL, "Failed to convert %s to basename", disk_name);
        free(path_buffer);
        return false;
    }

    snprintf(path_buffer, 256, "/sys/class/block/%s/stat", basename);
    if (access(path_buffer, F_OK) != 0) {
        ERR(ERRC_DeviceNotFound, NULL, "No disk or partition named '%s'", basename);
        free(path_buffer);
        return false;
    }

    io_stats *stats = read_io_stats(path_buffer);
    if (!stats) {
        ERR(ERRC_IOError, NULL, "Failed to read %s's io statistics", (const char*)basename);
        free(path_buffer);
        return false;
    }

    free(path_buffer);

    io_stats_fmt fmt_stats;
    if (!convert_stats_to_unit(stats, &fmt_stats)) {
        ERR(ERRC_AllocationFault, NULL, "Failed to allocate the I/O stats strings");
        free(stats);
        return false;
    }

    print_io_stats(basename, &fmt_stats);

    free_io_stats_fmt(&fmt_stats);
    free(stats);
    return true;
}   

/**
 * 
┌─────────────┬──────────────────────────────────────────┬─────────────┬────────────────────────────────────────────────┐
│ # (C index) │                  Field                   │ Your value  │                      Unit                      │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 1 ([0])     │ reads completed                          │ 86159       │ count                                          │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 2 ([1])     │ reads merged                             │ 24878       │ count                                          │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 3 ([2])     │ sectors read                             │ 7912014     │ × 512 bytes                                    │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 4 ([3])     │ time spent reading                       │ 15262       │ ms                                             │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 5 ([4])     │ writes completed                         │ 120464      │ count                                          │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 6 ([5])     │ writes merged                            │ 137368      │ count                                          │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 7 ([6])     │ sectors written                          │ 12145090    │ × 512 bytes                                    │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 8 ([7])     │ time spent writing                       │ 444173      │ ms                                             │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 9 ([8])     │ I/Os in flight right now                 │ 0           │ count, the only one that isn't a running total │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 10 ([9])    │ time the disk was busy                   │ 159372      │ ms                                             │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 11 ([10])   │ weighted time in queue                   │ 469571      │ ms                                             │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 12–15       │ discards: completed, merged, sectors, ms │ 0 0 0 0     │ kernel 4.18+                                   │
├─────────────┼──────────────────────────────────────────┼─────────────┼────────────────────────────────────────────────┤
│ 16–17       │ flushes: completed, ms                   │ 10405 10134 │ kernel 5.5+                                    │
└─────────────┴──────────────────────────────────────────┴─────────────┴────────────────────────────────────────────────┘
 */
