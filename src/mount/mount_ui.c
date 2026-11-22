#include "mount_ui.h"

#define SIZE_STR_SIZE 16

void print_mounted(const drive_mt *drive, const char *mountpoint, bool read_only, bool created) {
    char size[SIZE_STR_SIZE];
    ui_format_size(drive->size, size, sizeof(size));

    putchar('\n');
    printf("  %s%s✔%s %s%s%s mounted at %s%s%s  %s%s", BOLD, GREEN, RST, BOLD, drive->name, RST, BOLD, mountpoint, RST, GRAY, drive->fstype);
    if (drive->label[0]) printf(" \"%s\"", drive->label);
    printf(" · %s%s\n", size, RST);

    if (read_only) printf("    %s%s!%s mounted read-only, the drive is write-protected\n", BOLD, YELLOW, RST);
    if (created) printf("    %screated %s%s\n", GRAY, mountpoint, RST);
    printf("    %sunmount with%s %s%s -umnt %s%s\n\n", GRAY, RST, CYAN, ui_program_name, mountpoint, RST);
}
