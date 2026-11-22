#include "health_ui.h"
#include "../sectr_helper/sectr_helper.h"
#include <inttypes.h>

#define KEY_WIDTH 16
#define BAR_WIDTH 20
#define SIZE_STR_SIZE 16
#define ATTR_NAME_WIDTH 24

static const char *attr_name(uint8_t id) {
    switch (id) {
        case 1: return "Raw read error rate";
        case 3: return "Spin-up time";
        case 4: return "Start/stop count";
        case 5: return "Reallocated sectors";
        case 7: return "Seek error rate";
        case 9: return "Power-on hours";
        case 10: return "Spin retry count";
        case 12: return "Power cycles";
        case 170: return "Available reserved space";
        case 171: return "Program fail count";
        case 172: return "Erase fail count";
        case 173: return "Wear leveling count";
        case 174: return "Unexpected power loss";
        case 177: return "Wear leveling count";
        case 179: return "Used reserved blocks";
        case 180: return "Unused reserved blocks";
        case 181: return "Program fail count";
        case 182: return "Erase fail count";
        case 183: return "Runtime bad blocks";
        case 184: return "End-to-end errors";
        case 187: return "Reported uncorrectable";
        case 188: return "Command timeouts";
        case 189: return "High fly writes";
        case 190: return "Airflow temperature";
        case 191: return "G-sense errors";
        case 192: return "Power-off retracts";
        case 193: return "Load cycles";
        case 194: return "Temperature";
        case 195: return "Hardware ECC recovered";
        case 196: return "Reallocation events";
        case 197: return "Pending sectors";
        case 198: return "Offline uncorrectable";
        case 199: return "UDMA CRC errors";
        case 200: return "Write error rate";
        case 202: return "Percent lifetime left";
        case 231: return "SSD life left";
        case 232: return "Available reserved space";
        case 233: return "Media wearout indicator";
        case 240: return "Head flying hours";
        case 241: return "Total LBAs written";
        case 242: return "Total LBAs read";
        default: return "Vendor specific";
    }
}

static void print_header(const drive_mt *disk) {
    char name[DEVICE_NAME_SIZE], size[SIZE_STR_SIZE];
    device_basename(disk->name, name, sizeof(name));
    ui_format_size(disk->size, size, sizeof(size));

    printf("\n  %s%s▍ Health%s  %s%s%s", BOLD, PURPLE, RST, BOLD, name, RST);
    if (disk->model[0]) printf("  %s", disk->model);
    printf("  %s·  %s  ·  %s%s\n", GRAY, ui_bus_label(disk->bus), size, RST);
}

static void print_verdict(HEALTH_LEVEL level, const char *reason) {
    putchar('\n');
    if (level == HEALTH_OK) ui_ok("%sHealthy%s  %s%s%s", BOLD, RST, GRAY, reason, RST);
    else if (level == HEALTH_WARNING) printf("  %s%s!%s %s%sWarning%s  %s\n", BOLD, YELLOW, RST, BOLD, YELLOW, RST, reason);
    else printf("  %s%s✘%s %s%sFailing%s  %s\n", BOLD, RED, RST, BOLD, RED, RST, reason);
}

static void print_key(const char *key) {
    printf("    %s%-*s%s ", GRAY, KEY_WIDTH, key, RST);
}

static void print_u64(const char *key, uint64_t value) {
    print_key(key);
    printf("%" PRIu64 "\n", value);
}

static void print_bad_count(const char *key, uint64_t value) {
    print_key(key);
    if (value > 0) printf("%s%s%" PRIu64 "%s\n", BOLD, YELLOW, value, RST);
    else printf("%" PRIu64 "\n", value);
}

static void print_bytes(const char *key, uint64_t bytes) {
    char size[SIZE_STR_SIZE];
    ui_format_size(bytes, size, sizeof(size));
    print_key(key);
    printf("%s\n", size);
}

static void print_hours(const char *key, uint64_t hours) {
    print_key(key);
    printf("%" PRIu64 " h  %s(%" PRIu64 " days)%s\n", hours, GRAY, hours / 24, RST);
}

static void print_temperature(const char *key, int64_t celsius) {
    print_key(key);
    printf("%" PRId64 " °C\n", celsius);
}

void print_nvme_health(const drive_mt *disk, const nvme_health_t *health) {
    print_header(disk);
    print_verdict(health->level, health->reason);

    ui_section("Status");
    print_temperature("temperature", health->temperature);
    print_key("life used");
    ui_bar((double)health->percentage_used / 100.0, BAR_WIDTH);
    putchar('\n');
    print_key("spare");
    printf("%u%%  %s(warns below %u%%)%s\n", health->available_spare, GRAY, health->spare_threshold, RST);

    ui_section("Usage");
    print_hours("power on", health->power_on_hours);
    print_u64("power cycles", health->power_cycles);
    print_bytes("data read", health->data_read);
    print_bytes("data written", health->data_written);
    print_u64("read commands", health->read_commands);
    print_u64("write commands", health->write_commands);

    ui_section("Errors");
    print_bad_count("media errors", health->media_errors);
    print_u64("unsafe shutdowns", health->unsafe_shutdowns);
    print_u64("error log", health->error_log_entries);
    putchar('\n');
}

static void print_summary_count(const ata_smart_t *smart, uint8_t id, const char *key) {
    const ata_attr_t *attr = ata_find_attr(smart, id);
    if (attr) print_bad_count(key, ata_attr_raw(attr));
}

static void print_ata_summary(const ata_smart_t *smart) {
    const ata_attr_t *temperature = ata_find_attr(smart, ATA_ATTR_TEMPERATURE);
    if (!temperature) temperature = ata_find_attr(smart, ATA_ATTR_AIRFLOW_TEMP);
    if (temperature) print_temperature("temperature", (int64_t)ata_attr_raw(temperature));

    const ata_attr_t *hours = ata_find_attr(smart, ATA_ATTR_POWER_ON_HOURS);
    if (hours) print_hours("power on", ata_attr_raw(hours));

    const ata_attr_t *cycles = ata_find_attr(smart, ATA_ATTR_POWER_CYCLES);
    if (cycles) print_u64("power cycles", ata_attr_raw(cycles));

    print_summary_count(smart, ATA_ATTR_REALLOCATED, "reallocated");
    print_summary_count(smart, ATA_ATTR_PENDING, "pending");
    print_summary_count(smart, ATA_ATTR_OFFLINE_UNCORRECTABLE, "uncorrectable");
    print_summary_count(smart, ATA_ATTR_REPORTED_UNCORRECTABLE, "reported errors");
    print_summary_count(smart, ATA_ATTR_CRC_ERRORS, "cable errors");
}

static void print_attr_row(const ata_attr_t *attr) {
    const bool failing = ata_attr_failing(attr);

    printf("    %s%3u  ", failing ? RED : "", attr->id);
    ui_pad(attr_name(attr->id), ATTR_NAME_WIDTH);
    printf(" %5u %5u %6u  %" PRIu64 "%s", attr->value, attr->worst, attr->threshold, ata_attr_raw(attr), RST);
    if (failing) printf("  %s%s← failing%s", BOLD, RED, RST);
    putchar('\n');
}

void print_ata_smart(const drive_mt *disk, const ata_smart_t *smart) {
    print_header(disk);
    print_verdict(smart->level, smart->reason);
    if (!smart->checksum_ok) printf("  %s%s!%s %sthe SMART data checksum is wrong, values may be off%s\n", BOLD, YELLOW, RST, GRAY, RST);

    ui_section("Summary");
    print_ata_summary(smart);

    ui_section("Attributes");
    printf("%s    %3s  %-*s %5s %5s %6s  %s%s\n", GRAY, "ID", ATTR_NAME_WIDTH, "NAME", "VALUE", "WORST", "THRESH", "RAW", RST);
    for (size_t i = 0; i < smart->count; ++i) print_attr_row(&smart->attrs[i]);
    putchar('\n');
}
