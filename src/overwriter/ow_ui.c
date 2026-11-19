#include "ow_ui.h"
#include "../defs.h"
#include "../sectr_helper/sectr_helper.h"
#include "../ui/ui.h"
#include "../disk_data/disk_data.h"

#define PROGRESS_BAR_WIDTH 30
#define ERASE_COL_NAME 12
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

static void print_erase_line(const char *branch, size_t name_width, const drive_mt *drive) {
    char name[DEVICE_NAME_SIZE], size[16];
    device_basename(drive->name, name, sizeof(name));
    ui_format_size(drive->size, size, sizeof(size));

    // only the device itself is bold, its partitions are plain
    printf("  %s%s%s%s", GRAY, branch, RST, branch[0] ? "" : BOLD);
    ui_pad(name, name_width);
    printf("%s %*s", RST, ERASE_COL_SIZE, size);

    const char *kind = drive->parent[0] ? drive->fstype : drive->model;
    if (kind[0]) printf("  %s%s%s", GRAY, kind, RST);
    if (drive->parent[0] && drive->label[0]) printf("  %s\"%s\"%s", GRAY, drive->label, RST);
    putchar('\n');
}

void print_erase_list(const Vec *vec, const drive_mt *target) {
    const drive_mt *drives = vec->data;

    putchar('\n');
    print_erase_line("", ERASE_COL_NAME + 3, target);

    size_t children = 0;
    for (uint32_t i = 0; i < vec->element_count; ++i) children += strcmp(drives[i].parent, target->name) == 0;

    size_t printed = 0;
    for (uint32_t i = 0; i < vec->element_count; ++i) {
        if (strcmp(drives[i].parent, target->name) != 0) continue;
        print_erase_line(++printed == children ? "└─ " : "├─ ", ERASE_COL_NAME, &drives[i]);
    }
}
