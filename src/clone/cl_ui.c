#include "cl_ui.h"
#include "../sectr_helper/sectr_helper.h"
#include "../ui/ui.h"
#include "../security/prompt.h"
#include <inttypes.h>
#include <sys/param.h>

#define PROGRESS_BAR_WIDTH 24
#define PROGRESS_TAIL_WIDTH 47
#define PART_COL_SIZE 9
#define SIZE_STR_SIZE 16
#define TIME_STR_SIZE 32
#define NOTE_SIZE 128

static const char *square_color(size_t index) {
    const char *colors[] = {PURPLE, CYAN, YELLOW, GREEN, BLUE, MAGENTA};
    return colors[index % ARRAY_COUNT(colors)];
}

static void format_duration(double seconds, char *buf, size_t buf_size) {
    const uint64_t total = (uint64_t)(seconds + 0.5);

    if (total < 60) snprintf(buf, buf_size, "%" PRIu64 "s", total);
    else if (total < 3600) snprintf(buf, buf_size, "%" PRIu64 "m %02" PRIu64 "s", total / 60, total % 60);
    else snprintf(buf, buf_size, "%" PRIu64 "h %02" PRIu64 "m", total / 3600, total / 60 % 60);
}

static size_t count_partitions(const Vec *vec, const drive_mt *drive, size_t *name_width) {
    const drive_mt *drives = vec->data;
    char name[DEVICE_NAME_SIZE];
    size_t count = 0;

    *name_width = 0;
    for (uint32_t i = 0; i < vec->element_count; ++i) {
        if (strcmp(drives[i].parent, drive->name) != 0) continue;
        ++count;
        if (device_basename(drives[i].name, name, sizeof(name))) *name_width = MAX(*name_width, strlen(name));
    }
    return count;
}

static void print_drive(const char *label, const drive_mt *drive, uint64_t size, size_t partitions) {
    char size_str[SIZE_STR_SIZE];
    ui_format_size(size, size_str, sizeof(size_str));

    printf("    %s%-4s%s  %s%s%s%s", GRAY, label, RST, BOLD, PURPLE, drive->name, RST);

    if (drive->parent[0]) {
        if (drive->fstype[0]) printf("  %s%s%s", BOLD, drive->fstype, RST);
        if (drive->label[0]) printf(" %s\"%s\"%s", GRAY, drive->label, RST);
    } else if (drive->model[0]) {
        printf("  %s%s%s", BOLD, drive->model, RST);
    }

    fputs(GRAY, stdout);
    if (drive->bus[0]) printf(" · %s", ui_bus_label(drive->bus));
    printf(" · %s", size_str);

    if (drive->parent[0]) {
        char parent[DEVICE_NAME_SIZE];
        if (device_basename(drive->parent, parent, sizeof(parent))) printf(" · partition of %s", parent);
    } else if (partitions == 0) {
        fputs(" · no partitions", stdout);
    } else {
        printf(" · %zu partition%s", partitions, partitions == 1 ? "" : "s");
    }
    printf("%s\n", RST);
}

static void print_partitions(const Vec *vec, const drive_mt *drive, size_t name_width) {
    const drive_mt *drives = vec->data;
    char name[DEVICE_NAME_SIZE], size[SIZE_STR_SIZE];
    size_t printed = 0;

    for (uint32_t i = 0; i < vec->element_count; ++i) {
        const drive_mt *part = &drives[i];
        if (strcmp(part->parent, drive->name) != 0) continue;

        device_basename(part->name, name, sizeof(name));
        ui_format_size(part->size, size, sizeof(size));

        printf("          %s■%s ", square_color(printed++), RST);
        ui_pad(name, name_width);
        printf(" %*s", PART_COL_SIZE, size);

        if (part->fstype[0]) printf("  %s%s%s", GRAY, part->fstype, RST);
        if (part->label[0]) printf("  %s\"%s\"%s", GRAY, part->label, RST);
        putchar('\n');
    }
}

static void print_side(const char *label, const char *note, const char *note_color, const Vec *vec, const drive_mt *drive, uint64_t size) {
    size_t name_width = 0;
    const size_t partitions = count_partitions(vec, drive, &name_width);

    print_drive(label, drive, size, partitions);
    printf("          %s%s%s\n", note_color, note, RST);
    if (partitions > 0) print_partitions(vec, drive, name_width);
}

void cl_print_plan(const Vec *vec, const drive_mt *src, const drive_mt *dest, uint64_t src_size, uint64_t dest_size) {
    char src_name[DEVICE_NAME_SIZE], dest_name[DEVICE_NAME_SIZE], dest_note[NOTE_SIZE];
    device_basename(src->name, src_name, sizeof(src_name));
    device_basename(dest->name, dest_name, sizeof(dest_name));
    snprintf(dest_note, sizeof(dest_note), "becomes a copy of %s, its old data is replaced", src_name);

    printf("\n  %s%s▍ Clone%s  %s%s → %s%s\n\n", BOLD, PURPLE, RST, GRAY, src_name, dest_name, RST);
    print_side("from", "only read, nothing on it changes", GRAY, vec, src, src_size);
    putchar('\n');
    print_side("to", dest_note, YELLOW, vec, dest, dest_size);

    if (dest_size > src_size) {
        char rest[SIZE_STR_SIZE];
        ui_format_size(dest_size - src_size, rest, sizeof(rest));
        printf("\n    %s%s is bigger, its last %s stay as they are%s\n", GRAY, dest_name, rest, RST);
    }
}

bool cl_confirm_clone(const char *src_path, const char *dest_path, uint64_t dest_size) {
    char size[SIZE_STR_SIZE];
    ui_format_size(dest_size, size, sizeof(size));

    return prompt_confirm("%s%s!%s copy %s%s%s to %s%s%s%s %s(%s)%s?", BOLD, YELLOW, RST, BOLD, src_path, RST, BOLD, PURPLE, dest_path, RST, GRAY, size, RST);
}

void cl_print_progress(uint64_t done, uint64_t total, double seconds) {
    char done_str[SIZE_STR_SIZE], total_str[SIZE_STR_SIZE];
    ui_format_size(done, done_str, sizeof(done_str));
    ui_format_size(total, total_str, sizeof(total_str));

    char speed_part[80] = "";
    if (done > 0 && done < total && seconds >= 1.0) {
        const double speed = (double)done / seconds;
        char speed_str[SIZE_STR_SIZE], left_str[TIME_STR_SIZE];
        ui_format_size((uint64_t)speed, speed_str, sizeof(speed_str));
        format_duration((double)(total - done) / speed, left_str, sizeof(left_str));
        snprintf(speed_part, sizeof(speed_part), " · %s/s · %s left", speed_str, left_str);
    }

    char tail[128];
    snprintf(tail, sizeof(tail), "%s / %s%s", done_str, total_str, speed_part);

    printf("\r  ");
    ui_progress((double)done / (double)total, PROGRESS_BAR_WIDTH);
    printf("  %s", GRAY);
    ui_pad(tail, PROGRESS_TAIL_WIDTH);
    fputs(RST, stdout);
    fflush(stdout);
}

void cl_print_cancelled(void) {
    printf("  %scancelled, nothing was changed%s\n\n", GRAY, RST);
}

void cl_print_done(const char *src_path, const char *dest_path, uint64_t size, uint64_t dest_size, double seconds, bool replug_hint) {
    char size_str[SIZE_STR_SIZE], time_str[TIME_STR_SIZE], speed_str[SIZE_STR_SIZE];
    ui_format_size(size, size_str, sizeof(size_str));
    format_duration(seconds, time_str, sizeof(time_str));
    ui_format_size(seconds > 0.0 ? (uint64_t)((double)size / seconds) : size, speed_str, sizeof(speed_str));

    putchar('\n');
    ui_ok("%s%s%s is now a copy of %s%s%s  %s%s in %s · %s/s%s", BOLD, dest_path, RST, BOLD, src_path, RST, GRAY, size_str, time_str, speed_str, RST);
    printf("    %sboth have the same UUIDs now, so fstab and boot may pick either one%s\n", GRAY, RST);

    if (dest_size > size) {
        char rest[SIZE_STR_SIZE];
        ui_format_size(dest_size - size, rest, sizeof(rest));
        printf("    %sthe last %s of the target were left untouched%s\n", GRAY, rest, RST);
    }
    if (replug_hint) printf("    %sreplug the target or run partprobe to see its new partitions%s\n", GRAY, RST);
    putchar('\n');
}
