#pragma once

#include "../defs.h"

#define ATA_MAX_ATTRS 30
#define HEALTH_REASON_SIZE 128

#define ATA_ATTR_REALLOCATED 5
#define ATA_ATTR_POWER_ON_HOURS 9
#define ATA_ATTR_POWER_CYCLES 12
#define ATA_ATTR_REPORTED_UNCORRECTABLE 187
#define ATA_ATTR_AIRFLOW_TEMP 190
#define ATA_ATTR_TEMPERATURE 194
#define ATA_ATTR_PENDING 197
#define ATA_ATTR_OFFLINE_UNCORRECTABLE 198
#define ATA_ATTR_CRC_ERRORS 199

typedef enum {
    HEALTH_OK, HEALTH_WARNING, HEALTH_FAILING
} HEALTH_LEVEL;

typedef struct {
    uint8_t critical_warning;
    int32_t temperature;
    uint8_t available_spare;
    uint8_t spare_threshold;
    uint8_t percentage_used;
    uint64_t data_read;
    uint64_t data_written;
    uint64_t read_commands;
    uint64_t write_commands;
    uint64_t power_cycles;
    uint64_t power_on_hours;
    uint64_t unsafe_shutdowns;
    uint64_t media_errors;
    uint64_t error_log_entries;

    HEALTH_LEVEL level;
    char reason[HEALTH_REASON_SIZE];
} nvme_health_t;

typedef struct {
    uint8_t id;
    uint8_t flags;
    uint8_t value;
    uint8_t worst;
    uint8_t threshold;
    uint64_t raw;
} ata_attr_t;

typedef struct {
    bool status_failing;
    bool checksum_ok;
    size_t count;
    ata_attr_t attrs[ATA_MAX_ATTRS];

    HEALTH_LEVEL level;
    char reason[HEALTH_REASON_SIZE];
} ata_smart_t;

const ata_attr_t *ata_find_attr(const ata_smart_t *smart, uint8_t id);

/**
 * @brief The raw value as it should be shown: temperature is only the low byte, power-on hours the low 32 bits.
 */
uint64_t ata_attr_raw(const ata_attr_t *attr);

bool ata_attr_failing(const ata_attr_t *attr);

/**
 * @brief Reads the SMART / health data of a SATA or NVMe disk and prints it with a verdict.
 * A partition name ("sda1") reads the disk it is on. Needs root.
 * @param disk_name "sda", "/dev/sda" or "/dev/sda/"
 * @return true when the data was read and printed, false on error (ERR is set)
 */
bool disk_health(const char *disk_name);
