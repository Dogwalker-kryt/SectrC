#include "io_ui.h"

#define KEY_WIDTH 12

static void print_field(const char *key, const char *value) {
    const char *rest = strstr(value, " (");
    if (!rest) rest = strchr(value, ' ');

    printf("    %s%-*s%s ", GRAY, KEY_WIDTH, key, RST);
    if (!rest) {
        printf("%s\n", value);
        return;
    }

    if (strcmp(rest, "count") == 0) rest = " ";

    printf("%.*s%s%s%s\n", (int32_t)(rest - value), value, GRAY, rest, RST);
}

void print_io_stats(const char *disk_name, const io_stats_fmt *fmt_stats) {
    printf("\n  %s%s▍ I/O stats%s  %s%s%s  %s·  since boot%s\n", BOLD, PURPLE, RST, BOLD, disk_name, RST, GRAY, RST);

    ui_section("Reads");
    print_field("completed", fmt_stats->data[completed_reads]);
    print_field("merged", fmt_stats->data[reads_merged]);
    print_field("data", fmt_stats->data[sectors_read]);
    print_field("time", fmt_stats->data[reading_time_ms]);

    ui_section("Writes");
    print_field("completed", fmt_stats->data[completed_writes]);
    print_field("merged", fmt_stats->data[writes_merged]);
    print_field("data", fmt_stats->data[sectors_written]);
    print_field("time", fmt_stats->data[writing_time_ms]);

    ui_section("Discards");
    print_field("completed", fmt_stats->data[discards_completed]);
    print_field("merged", fmt_stats->data[discards_merged]);
    print_field("data", fmt_stats->data[discards_sectors]);
    print_field("time", fmt_stats->data[discards_time_ms]);

    ui_section("Queue");
    print_field("in flight", fmt_stats->data[io_in_flight]);
    print_field("busy", fmt_stats->data[time_disk_busy_ms]);
    print_field("queue time", fmt_stats->data[weighted_time_in_queue_ms]);
    putchar('\n');
}
