#include "health.h"
#include "health_ui.h"
#include "../defs.h"
#include "../disk_data/disk_data.h"
#include "../Debug/sectr_err.h"
#include "../sectr_helper/sectr_helper.h"
#include "../security/permission.h"
#include "../ui/ui.h"
#include <errno.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <linux/nvme_ioctl.h>
#include <linux/hdreg.h>

#define SMART_LOG_SIZE 512

#define NVME_ADMIN_GET_LOG_PAGE 0x02
#define NVME_LOG_SMART 0x02
#define NVME_NSID_ALL 0xFFFFFFFF
#define NVME_DATA_UNIT 512000ULL
#define KELVIN_OFFSET 273

#define NVME_OFF_CRITICAL_WARNING 0
#define NVME_OFF_TEMPERATURE 1
#define NVME_OFF_SPARE 3
#define NVME_OFF_SPARE_THRESHOLD 4
#define NVME_OFF_PERCENTAGE_USED 5
#define NVME_OFF_DATA_READ 32
#define NVME_OFF_DATA_WRITTEN 48
#define NVME_OFF_READ_COMMANDS 64
#define NVME_OFF_WRITE_COMMANDS 80
#define NVME_OFF_POWER_CYCLES 112
#define NVME_OFF_POWER_ON_HOURS 128
#define NVME_OFF_UNSAFE_SHUTDOWNS 144
#define NVME_OFF_MEDIA_ERRORS 160
#define NVME_OFF_ERROR_LOG_ENTRIES 176

#define NVME_WARN_TEMPERATURE 1
#define NVME_WARN_BITS 6
#define NVME_WEAR_WARNING 90

#define HDIO_HEADER_SIZE 4
#define ATA_SMART_CMD 0xB0
#define ATA_SMART_READ_VALUES 0xD0
#define ATA_SMART_READ_THRESHOLDS 0xD1
#define ATA_SMART_STATUS 0xDA
#define ATA_SMART_LBAM_PASS 0x4F
#define ATA_SMART_LBAH_PASS 0xC2
#define ATA_SMART_LBAM_FAIL 0xF4
#define ATA_SMART_LBAH_FAIL 0x2C
#define ATA_ATTR_TABLE_OFFSET 2
#define ATA_ATTR_ENTRY_SIZE 12

// TODO: make this in sectr_helper
static const drive_mt *find_drive(const Vec *vec, const char *wanted) {
    const drive_mt *drives = vec->data;
    char name[DEVICE_NAME_SIZE];

    for (uint32_t i = 0; i < vec->element_count; ++i) {
        if (device_basename(drives[i].name, name, sizeof(name)) && strcmp(name, wanted) == 0) return &drives[i];
    }
    return NULL;
}

static const drive_mt *find_whole_disk(const Vec *vec, const char *basename) {
    const drive_mt *target = find_drive(vec, basename);
    if (!target) {
        ERR(ERRC_DeviceNotFound, NULL, "No disk or partition named '%s'", basename);
        return NULL;
    }
    if (target->parent[0] == '\0') return target;

    char parent[DEVICE_NAME_SIZE];
    const drive_mt *disk = device_basename(target->parent, parent, sizeof(parent)) ? find_drive(vec, parent) : NULL;
    if (!disk) ERR(ERRC_MissingData, NULL, "Couldnt find the disk that %s is on", target->name);
    return disk;
}



typedef enum {
    BUS_ATA, BUS_NVME, BUS_UNKNOWN
} BUS_TYPE;

static BUS_TYPE check_BUS(const char *bus) {
    if (strcmp(bus, "ata") == 0) {
        return BUS_ATA;
    } else if (strcmp(bus, "nvme") == 0) {
        return BUS_NVME;
    }

    return BUS_UNKNOWN;
}

static uint64_t read_le(const uint8_t *bytes, size_t count) {
    uint64_t value = 0;
    for (size_t i = count; i > 0; --i) value = value << 8 | bytes[i - 1];
    return value;
}


static bool nvme_get_smart_log(fd_t fd, const char *path, uint8_t *log) {
    struct nvme_admin_cmd cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_GET_LOG_PAGE;
    cmd.nsid = NVME_NSID_ALL;
    cmd.addr = (uint64_t)(uintptr_t)log;
    cmd.data_len = SMART_LOG_SIZE;
    cmd.cdw10 = ((uint32_t)(SMART_LOG_SIZE / 4 - 1) << 16) | NVME_LOG_SMART;

    const int32_t result = ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd);
    if (result < 0) {
        ERR(ERRC_IOError, "Failed to read the NVMe health log", "%s: %s", path, strerror(errno));
        return false;
    }
    if (result > 0) {
        ERR(ERRC_IOError, "Failed to read the NVMe health log", "%s answered with NVMe status 0x%x", path, (uint32_t)result);
        return false;
    }
    return true;
}

static void nvme_parse(const uint8_t *log, nvme_health_t *health) {
    health->critical_warning = log[NVME_OFF_CRITICAL_WARNING];
    health->temperature = (int32_t)read_le(log + NVME_OFF_TEMPERATURE, 2) - KELVIN_OFFSET;
    health->available_spare = log[NVME_OFF_SPARE];
    health->spare_threshold = log[NVME_OFF_SPARE_THRESHOLD];
    health->percentage_used = log[NVME_OFF_PERCENTAGE_USED];
    health->data_read = read_le(log + NVME_OFF_DATA_READ, 8) * NVME_DATA_UNIT;
    health->data_written = read_le(log + NVME_OFF_DATA_WRITTEN, 8) * NVME_DATA_UNIT;
    health->read_commands = read_le(log + NVME_OFF_READ_COMMANDS, 8);
    health->write_commands = read_le(log + NVME_OFF_WRITE_COMMANDS, 8);
    health->power_cycles = read_le(log + NVME_OFF_POWER_CYCLES, 8);
    health->power_on_hours = read_le(log + NVME_OFF_POWER_ON_HOURS, 8);
    health->unsafe_shutdowns = read_le(log + NVME_OFF_UNSAFE_SHUTDOWNS, 8);
    health->media_errors = read_le(log + NVME_OFF_MEDIA_ERRORS, 8);
    health->error_log_entries = read_le(log + NVME_OFF_ERROR_LOG_ENTRIES, 8);
}

static void nvme_verdict(nvme_health_t *health) {
    static const char *const warning_names[NVME_WARN_BITS] = {
        "spare space is below its threshold", "temperature is out of range", "reliability is degraded",
        "the drive switched to read-only", "volatile memory backup failed", "persistent memory is read-only",
    };

    health->level = HEALTH_OK;
    snprintf(health->reason, sizeof(health->reason), "no problems reported");

    for (uint8_t bit = 0; bit < NVME_WARN_BITS; ++bit) {
        if (bit == NVME_WARN_TEMPERATURE || !(health->critical_warning & (1u << bit))) continue;
        health->level = HEALTH_FAILING;
        snprintf(health->reason, sizeof(health->reason), "%s", warning_names[bit]);
        return;
    }

    if (health->critical_warning & (1u << NVME_WARN_TEMPERATURE)) {
        health->level = HEALTH_WARNING;
        snprintf(health->reason, sizeof(health->reason), "%s", warning_names[NVME_WARN_TEMPERATURE]);
    } else if (health->media_errors > 0) {
        health->level = HEALTH_WARNING;
        snprintf(health->reason, sizeof(health->reason), "%" PRIu64 " media error%s", health->media_errors, health->media_errors == 1 ? "" : "s");
    } else if (health->percentage_used >= NVME_WEAR_WARNING) {
        health->level = HEALTH_WARNING;
        snprintf(health->reason, sizeof(health->reason), "%u%% of the rated write endurance is used", health->percentage_used);
    }
}

static bool read_nvme_health(fd_t fd, const char *path, nvme_health_t *health) {
    uint8_t log[SMART_LOG_SIZE] = {0};
    if (!nvme_get_smart_log(fd, path, log)) return false;

    nvme_parse(log, health);
    nvme_verdict(health);
    return true;
}


static bool ata_smart_cmd(fd_t fd, uint8_t feature, uint8_t *buffer) {
    memset(buffer, 0, HDIO_HEADER_SIZE + SMART_LOG_SIZE);
    buffer[0] = ATA_SMART_CMD;
    buffer[2] = feature;
    buffer[3] = 1;
    return ioctl(fd, HDIO_DRIVE_CMD, buffer) == 0;
}

static bool ata_smart_status(fd_t fd, bool *failing) {
    uint8_t task[7] = {ATA_SMART_CMD, ATA_SMART_STATUS, 0, 0, ATA_SMART_LBAM_PASS, ATA_SMART_LBAH_PASS, 0};
    if (ioctl(fd, HDIO_DRIVE_TASK, task) != 0) return false;

    *failing = task[4] == ATA_SMART_LBAM_FAIL && task[5] == ATA_SMART_LBAH_FAIL;
    return true;
}

static uint8_t ata_threshold(const uint8_t *thresholds, uint8_t id) {
    for (size_t i = 0; i < ATA_MAX_ATTRS; ++i) {
        const uint8_t *entry = thresholds + ATA_ATTR_TABLE_OFFSET + i * ATA_ATTR_ENTRY_SIZE;
        if (entry[0] == id) return entry[1];
    }
    return 0;
}

static void ata_parse(const uint8_t *values, const uint8_t *thresholds, ata_smart_t *smart) {
    uint8_t sum = 0;
    for (size_t i = 0; i < SMART_LOG_SIZE; ++i) sum = (uint8_t)(sum + values[i]);
    smart->checksum_ok = sum == 0;

    smart->count = 0;
    for (size_t i = 0; i < ATA_MAX_ATTRS; ++i) {
        const uint8_t *entry = values + ATA_ATTR_TABLE_OFFSET + i * ATA_ATTR_ENTRY_SIZE;
        if (entry[0] == 0) continue;

        ata_attr_t *attr = &smart->attrs[smart->count++];
        attr->id = entry[0];
        attr->flags = entry[1];
        attr->value = entry[3];
        attr->worst = entry[4];
        attr->raw = read_le(entry + 5, 6);
        attr->threshold = ata_threshold(thresholds, attr->id);
    }
}

const ata_attr_t *ata_find_attr(const ata_smart_t *smart, uint8_t id) {
    for (size_t i = 0; i < smart->count; ++i) {
        if (smart->attrs[i].id == id) return &smart->attrs[i];
    }
    return NULL;
}

uint64_t ata_attr_raw(const ata_attr_t *attr) {
    if (attr->id == ATA_ATTR_TEMPERATURE || attr->id == ATA_ATTR_AIRFLOW_TEMP) return attr->raw & 0xFF;
    if (attr->id == ATA_ATTR_POWER_ON_HOURS) return attr->raw & 0xFFFFFFFF;
    return attr->raw;
}

bool ata_attr_failing(const ata_attr_t *attr) {
    return attr->threshold != 0 && attr->value <= attr->threshold;
}

static void ata_verdict(ata_smart_t *smart) {
    static const struct { uint8_t id; const char *what; } bad_counters[] = {
        {ATA_ATTR_REALLOCATED, "reallocated sector"},
        {ATA_ATTR_PENDING, "pending sector"},
        {ATA_ATTR_OFFLINE_UNCORRECTABLE, "uncorrectable sector"},
        {ATA_ATTR_REPORTED_UNCORRECTABLE, "reported uncorrectable error"},
    };

    smart->level = HEALTH_OK;
    snprintf(smart->reason, sizeof(smart->reason), "no problems reported");

    if (smart->status_failing) {
        smart->level = HEALTH_FAILING;
        snprintf(smart->reason, sizeof(smart->reason), "the drive reports that it is failing");
        return;
    }

    for (size_t i = 0; i < smart->count; ++i) {
        const ata_attr_t *attr = &smart->attrs[i];
        if (!ata_attr_failing(attr)) continue;
        smart->level = HEALTH_FAILING;
        snprintf(smart->reason, sizeof(smart->reason), "attribute %u is at or below its threshold (%u <= %u)", attr->id, attr->value, attr->threshold);
        return;
    }

    for (size_t i = 0; i < SIZEOF_ARRAY(bad_counters); ++i) {
        const ata_attr_t *attr = ata_find_attr(smart, bad_counters[i].id);
        if (!attr || attr->raw == 0) continue;
        smart->level = HEALTH_WARNING;
        snprintf(smart->reason, sizeof(smart->reason), "%" PRIu64 " %s%s", attr->raw, bad_counters[i].what, attr->raw == 1 ? "" : "s");
        return;
    }
}

static bool read_ata_smart(fd_t fd, const char *path, ata_smart_t *smart) {
    uint8_t values[HDIO_HEADER_SIZE + SMART_LOG_SIZE];
    uint8_t thresholds[HDIO_HEADER_SIZE + SMART_LOG_SIZE];
    bool failing = false;

    if (!ata_smart_cmd(fd, ATA_SMART_READ_VALUES, values)) {
        ERR(ERRC_IOError, "Failed to read SMART data", "%s: %s (is SMART turned off in the drive?)", path, strerror(errno));
        return false;
    }
    if (!ata_smart_cmd(fd, ATA_SMART_READ_THRESHOLDS, thresholds)) memset(thresholds, 0, sizeof(thresholds));
    if (!ata_smart_status(fd, &failing)) {
        ERR(ERRC_IOError, "Failed to read the SMART status", "%s: %s", path, strerror(errno));
        return false;
    }

    ata_parse(values + HDIO_HEADER_SIZE, thresholds + HDIO_HEADER_SIZE, smart);
    smart->status_failing = failing;
    ata_verdict(smart);
    return true;
}

// TODO: in vec_init e.g embedd ERR() so that if something fails, you just use the if statement to return and then PERR() and the controll flwo does the rest

bool disk_health(const char *disk_name) {
    if (!is_root()) {
        ERR(ERRC_PermissionDenied, NULL, "Reading SMART data requires root privileges. Please run with 'sudo'");
        return false;
    }

    fd_t fd = -1;

    Vec vec;
    if (!vec_init(&vec, DEFAULT_EXPECTED_ELEMENTS_COUNT, sizeof(drive_mt))) return false;
    if (!get_physical_disk_info(&vec)) {
        goto _exit_failure;
    }

    char disk_basename[DEVICE_NAME_SIZE];
    if (!device_basename(disk_name, disk_basename, sizeof(disk_basename))) {
        ERR(ERRC_InvalidInput, NULL, "'%s' is not a valid disk name", disk_name);
        goto _exit_failure;
    }

    const drive_mt *disk = find_whole_disk(&vec, disk_basename);
    if (!disk) {
        goto _exit_failure;
    }

    const BUS_TYPE bus_type = check_BUS(disk->bus);
    if (bus_type == BUS_UNKNOWN) {
        ERR(ERRC_InvalidDevice, "SMART is not supported for this disk", "%s is a %s disk, only SATA and NVMe disks are supported", disk->name, disk->bus[0] ? ui_bus_label(disk->bus) : disk->type);
        goto _exit_failure;
    }

    fd = open(disk->name, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        if (errno == EACCES || errno == EPERM) ERR(ERRC_PermissionDenied, NULL, "Cannot open %s, run it with sudo", disk->name);
        else ERR(ERRC_IOError, "Failed to open drive for reading SMART data", "%s: %s", disk->name, strerror(errno));
        goto _exit_failure;
    }

    if (bus_type == BUS_NVME) {
        nvme_health_t health;
        if (!read_nvme_health(fd, disk->name, &health)) {
            goto _exit_failure;
        }
        print_nvme_health(disk, &health);
    } else {
        ata_smart_t smart;
        if (!read_ata_smart(fd, disk->name, &smart)) {
            goto _exit_failure;
        }
        print_ata_smart(disk, &smart);
    }

    close(fd);
    vec_free(&vec);
    return true;

_exit_failure:
    if (fd >= 0) close(fd);
    vec_free(&vec);
    return false;
}
