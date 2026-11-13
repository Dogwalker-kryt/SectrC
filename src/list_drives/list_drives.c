#include "list_drives.h"
#include "../sectr_helper/sectr_helper.h"
#include "../ui/ui.h"
#include <inttypes.h>
#include <sys/statvfs.h>

#define COL_NAME  12
#define COL_FS    6
#define COL_SIZE  9
#define COL_MOUNT 14
#define BAR_WIDTH 12

static const char *device_basename(const char *device_name) {
    const char *basename = strrchr(device_name, '/');
    return basename ? basename + 1 : device_name;
}

static bool is_partition(const drive_mt *drive) {
    return strcmp(drive->type, "partition") == 0;
}

static const char *or_dash(const char *value) {
    return value && value[0] ? value : "-";
}

static bool get_usage(const char *mountpoint, uint64_t *used, uint64_t *total, double *fraction) {
    struct statvfs st;
    if (!mountpoint || mountpoint[0] != '/' || statvfs(mountpoint, &st) != 0 || st.f_blocks == 0) return false;

    *total = (uint64_t)st.f_blocks * st.f_frsize;
    *used = *total - (uint64_t)st.f_bfree * st.f_frsize;
    uint64_t available = (uint64_t)st.f_bavail * st.f_frsize;
    *fraction = (*used + available) ? (double)*used / (double)(*used + available) : 0.0;
    return true;
}

static const char *bus_label(const char *bus) {
    if (strcmp(bus, "nvme") == 0) return "NVMe";
    if (strcmp(bus, "ata") == 0) return "SATA";
    if (strcmp(bus, "usb") == 0) return "USB";
    if (strcmp(bus, "scsi") == 0) return "SCSI";
    return bus;
}

static const char *mount_label(const drive_mt *drive) {
    if (drive->mountpoint[0]) return drive->mountpoint;
    if (strcmp(drive->fstype, "swap") == 0) return "[swap]";
    return "-";
}

static void print_tree_row(const drive_mt *part, bool last) {
    char size[16];
    ui_format_size(part->size, size, sizeof(size));

    printf("    %s%s%s ", GRAY, last ? "└─" : "├─", RST);
    fputs(BOLD, stdout);
    ui_pad(device_basename(part->name), COL_NAME);
    printf("%s %s", RST, CYAN);
    ui_pad(or_dash(part->fstype), COL_FS);
    printf("%s %*s  ", RST, COL_SIZE, size);

    bool mounted = part->mountpoint[0] != '\0';
    fputs(mounted ? "" : GRAY, stdout);
    ui_pad(mount_label(part), COL_MOUNT);
    fputs(RST, stdout);

    uint64_t used, total;
    double fraction;
    if (mounted && get_usage(part->mountpoint, &used, &total, &fraction)) {
        putchar(' ');
        ui_bar(fraction, BAR_WIDTH);
    }
    putchar('\n');
}

static void print_tree_header(void) {
    printf("%s       %-*s %-*s %*s  %-*s USAGE%s\n", GRAY, COL_NAME, "NAME", COL_FS, "FS", COL_SIZE, "SIZE", COL_MOUNT, "MOUNT", RST);
}

static void print_disk_header(const drive_mt *disk) {
    char size[16];
    ui_format_size(disk->size, size, sizeof(size));

    printf("\n  %s%s●%s %s%s%s  %s  %s%s%s\n", BOLD, CYAN, RST, BOLD, device_basename(disk->name), RST, or_dash(disk->model), BOLD, size, RST);

    printf("    %s", GRAY);
    printf("%s", disk->bus[0] ? bus_label(disk->bus) : disk->type);
    if (disk->vendor[0]) printf(" · %s", disk->vendor);
    if (disk->serial[0]) printf(" · serial %s", disk->serial);
    printf("%s\n", RST);
}

static void print_overview(const drive_mt *disks, size_t count) {
    size_t disk_count = 0, part_count = 0;

    for (size_t i = 0; i < count; ++i) {
        const drive_mt *disk = &disks[i];
        if (is_partition(disk)) continue;
        ++disk_count;

        print_disk_header(disk);

        size_t children = 0;
        for (size_t j = 0; j < count; ++j) children += is_partition(&disks[j]) && strcmp(disks[j].parent, disk->name) == 0;
        part_count += children;

        if (children == 0 && disk->fstype[0]) {
            print_tree_header();
            print_tree_row(disk, true);
            continue;
        }
        if (children == 0) {
            printf("    %s└─ no partitions%s\n", GRAY, RST);
            continue;
        }

        print_tree_header();
        size_t printed = 0;
        for (size_t j = 0; j < count; ++j) {
            if (!is_partition(&disks[j]) || strcmp(disks[j].parent, disk->name) != 0) continue;
            print_tree_row(&disks[j], ++printed == children);
        }
    }

    printf("\n  %s%zu %s · %zu %s   ·   %s", GRAY, disk_count, disk_count == 1 ? "disk" : "disks", part_count, part_count == 1 ? "partition" : "partitions", RST);
    printf("%s%s -l <name>%s %sfor details%s\n\n", CYAN, ui_program_name, RST, GRAY, RST);
}

static void print_field(const char *key, const char *value) {
    printf("    %s%-12s%s %s%s%s\n", GRAY, key, RST, value && value[0] ? "" : GRAY, or_dash(value), RST);
}

static void print_card(const drive_mt *drive) {
    char size[16], line[160];
    ui_format_size(drive->size, size, sizeof(size));

    if (is_partition(drive)) printf("\n  %s%s●%s %s%s%s  %spartition of %s%s\n", BOLD, MAGENTA, RST, BOLD, drive->name, RST, GRAY, device_basename(drive->parent), RST);
    else printf("\n  %s%s●%s %s%s%s  %s%s disk%s\n", BOLD, CYAN, RST, BOLD, drive->name, RST, GRAY, drive->bus[0] ? bus_label(drive->bus) : drive->type, RST);

    printf("    %s──────────────────────────────────────────────────%s\n", GRAY, RST);

    snprintf(line, sizeof(line), "%s%s%s  %s(%" PRIu64 " bytes)%s", BOLD, size, RST, GRAY, drive->size, RST);
    print_field("Size", line);
    print_field("Filesystem", drive->fstype);
    print_field("Label", drive->label);
    print_field("UUID", drive->uuid);
    print_field("Mounted on", drive->mountpoint[0] ? drive->mountpoint : strcmp(drive->fstype, "swap") == 0 ? "[swap]" : "");

    uint64_t used, total;
    double fraction;
    if (drive->mountpoint[0] && get_usage(drive->mountpoint, &used, &total, &fraction)) {
        char used_str[16], total_str[16];
        ui_format_size(used, used_str, sizeof(used_str));
        ui_format_size(total, total_str, sizeof(total_str));
        printf("    %s%-12s%s ", GRAY, "Usage", RST);
        ui_bar(fraction, 20);
        printf("  %s%s used of %s%s\n", GRAY, used_str, total_str, RST);
    }

    print_field("Model", drive->model);
    print_field("Vendor", drive->vendor);
    print_field("Bus", drive->bus[0] ? bus_label(drive->bus) : "");
    print_field("Serial", drive->serial);
    print_field("Status", drive->status);
}

bool print_disk_infos(const drive_mt *disks, size_t count, const char *disk_name) {
    if (!disk_name || disk_name[0] == '\0') {
        print_overview(disks, count);
        return count > 0;
    }

    const char *wanted = device_basename(disk_name);
    size_t printed_count = 0;

    for (size_t i = 0; i < count; i++) {
        bool matches = strcmp(wanted, device_basename(disks[i].name)) == 0 || (disks[i].parent[0] != '\0' && strcmp(wanted, device_basename(disks[i].parent)) == 0);
        if (!matches) continue;

        print_card(&disks[i]);
        ++printed_count;
    }

    if (printed_count > 0) putchar('\n');
    return printed_count > 0;
}
