#include "ow_ui.h"
#include "../defs.h"
#include "../sectr_helper/sectr_helper.h"
#include "../ui/ui.h"
#include "../security/prompt.h"
#include "../disk_data/disk_data.h"
#include <sys/param.h>

#define PROGRESS_BAR_WIDTH 30
#define ERASE_COL_SIZE 9

void print_progress(uint64_t done, uint64_t total) {
    char done_str[16], total_str[16];
    ui_format_size(done, done_str, sizeof(done_str));
    ui_format_size(total, total_str, sizeof(total_str));

    printf("\r  ");
    ui_progress((double)done / (double)total, PROGRESS_BAR_WIDTH);
    printf("  %s%s / %s%s   ", GRAY, done_str, total_str, RST);
    fflush(stdout);
}

static const char *square_color(size_t index) {
    const char *colors[] = {PURPLE, CYAN, YELLOW, GREEN, BLUE, MAGENTA};
    return colors[index % SIZEOF_ARRAY(colors)];
}

static void print_erase_header(const drive_mt *target, size_t children) {
    char size[16];
    ui_format_size(target->size, size, sizeof(size));

    printf("\n  %s%s%s%s", BOLD, PURPLE, target->name, RST);

    if (target->parent[0]) {
        if (target->fstype[0]) printf("  %s%s%s", BOLD, target->fstype, RST);
        if (target->label[0]) printf(" %s\"%s\"%s", GRAY, target->label, RST);
    } else if (target->model[0]) {
        printf("  %s%s%s", BOLD, target->model, RST);
    }

    fputs(GRAY, stdout);
    if (target->bus[0]) printf(" · %s", ui_bus_label(target->bus));
    printf(" · %s", size);

    if (target->parent[0]) {
        char parent[DEVICE_NAME_SIZE];
        if (device_basename(target->parent, parent, sizeof(parent))) printf(" · partition of %s", parent);
    } else if (children == 0) {
        fputs(" · no partitions", stdout);
    } else {
        printf(" · %zu partition%s", children, children == 1 ? "" : "s");
    }
    printf("%s\n", RST);
}

static void print_erase_line(size_t index, size_t name_width, const drive_mt *drive) {
    char name[DEVICE_NAME_SIZE], size[16];
    device_basename(drive->name, name, sizeof(name));
    ui_format_size(drive->size, size, sizeof(size));

    printf("  %s■%s ", square_color(index), RST);
    ui_pad(name, name_width);
    printf(" %*s", ERASE_COL_SIZE, size);

    if (drive->fstype[0]) printf("  %s%s%s", GRAY, drive->fstype, RST);
    if (drive->label[0]) printf("  %s\"%s\"%s", GRAY, drive->label, RST);
    putchar('\n');
}

void print_erase_list(const Vec *vec, const drive_mt *target) {
    const drive_mt *drives = vec->data;
    char name[DEVICE_NAME_SIZE];

    size_t children = 0, name_width = 0;
    for (uint32_t i = 0; i < vec->element_count; ++i) {
        if (strcmp(drives[i].parent, target->name) != 0) continue;
        ++children;
        if (device_basename(drives[i].name, name, sizeof(name))) name_width = MAX(name_width, strlen(name));
    }

    print_erase_header(target, children);
    if (children == 0) return;

    putchar('\n');
    size_t printed = 0;
    for (uint32_t i = 0; i < vec->element_count; ++i) {
        if (strcmp(drives[i].parent, target->name) == 0) print_erase_line(printed++, name_width, &drives[i]);
    }
}

bool ow_confirm_erase(const char *device_path, const char *size_str) {
    return prompt_confirm("%s%s!%s erase %sall data%s on %s%s%s%s %s(%s)%s?", BOLD, YELLOW, RST, BOLD, RST, BOLD, PURPLE, device_path, RST, GRAY, size_str, RST);
}

void ow_print_cancelled(void) {
    printf("  %scancelled, nothing was changed%s\n\n", GRAY, RST);
}

void ow_print_done(const char *device_path, const char *size_str, const char *method) {
    putchar('\n');
    ui_ok("%s%s%s erased  %s%s, %s%s", BOLD, device_path, RST, GRAY, size_str, method, RST);
    putchar('\n');
}
