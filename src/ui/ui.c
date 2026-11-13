#include "ui.h"
#include <inttypes.h>

const char *ui_program_name = "sectr";

#define BANNER_LINES 6

static const char *const banner_lines[BANNER_LINES] = {
    "███████╗███████╗ ██████╗████████╗██████╗  ██████╗",
    "██╔════╝██╔════╝██╔════╝╚══██╔══╝██╔══██╗██╔════╝",
    "███████╗█████╗  ██║        ██║   ██████╔╝██║     ",
    "╚════██║██╔══╝  ██║        ██║   ██╔══██╗██║     ",
    "███████║███████╗╚██████╗   ██║   ██║  ██║╚██████╗",
    "╚══════╝╚══════╝ ╚═════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝",
};

/* Same cyan -> purple gradient as setup.sh. */
static const char *const banner_colors[BANNER_LINES] = {
    "\033[1;38;5;51m", "\033[1;38;5;45m", "\033[1;38;5;39m",
    "\033[1;38;5;63m", "\033[1;38;5;99m", "\033[1;38;5;135m",
};

void ui_banner(void) {
    putchar('\n');
    for (size_t i = 0; i < BANNER_LINES; ++i) printf("  %s%s%s\n", sectr_color(banner_colors[i]), banner_lines[i], RST);
}

void ui_format_size(uint64_t bytes, char *buf, size_t buf_size) {
    static const char *const units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    double value = (double)bytes;
    size_t unit = 0;

    while (value >= 1024.0 && unit < SIZEOF_ARRAY(units) - 1) {
        value /= 1024.0;
        ++unit;
    }

    if (unit == 0) {
        snprintf(buf, buf_size, "%" PRIu64 " B", bytes);
        return;
    }

    int precision = value >= 100.0 ? 0 : value >= 10.0 ? 1 : 2;
    snprintf(buf, buf_size, "%.*f %s", precision, value, units[unit]);
}

void ui_tilde_path(const char *path, char *buf, size_t buf_size) {
    const char *home = getenv("HOME");
    size_t home_len = home ? strlen(home) : 0;

    if (home_len > 1 && strncmp(path, home, home_len) == 0 && (path[home_len] == '/' || path[home_len] == '\0')) snprintf(buf, buf_size, "~%s", path + home_len);
    else snprintf(buf, buf_size, "%s", path);
}

static bool is_utf8_start(char c) {
    return ((unsigned char)c & 0xC0) != 0x80;
}

static size_t utf8_columns(const char *text) {
    size_t columns = 0;
    for (; *text; ++text) columns += is_utf8_start(*text);
    return columns;
}

void ui_pad(const char *text, size_t width) {
    if (width == 0) return;

    size_t columns = utf8_columns(text);
    if (columns <= width) {
        printf("%s%*s", text, (int)(width - columns), "");
        return;
    }

    size_t shown = 0;
    for (const char *p = text; *p; ++p) {
        if (is_utf8_start(*p) && shown++ == width - 1) break;
        putchar(*p);
    }
    fputs("…", stdout);
}

void ui_bar(double fraction, size_t width) {
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;

    size_t filled = (size_t)(fraction * (double)width + 0.5);
    const char *color = fraction < 0.70 ? GREEN : fraction < 0.90 ? YELLOW : RED;

    fputs(color, stdout);
    for (size_t i = 0; i < filled; ++i) fputs("█", stdout);
    fputs(GRAY, stdout);
    for (size_t i = filled; i < width; ++i) fputs("░", stdout);
    printf("%s %s%s%3.0f%%%s", RST, BOLD, color, fraction * 100.0, RST);
}

void ui_ok(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("  %s%s✔%s ", BOLD, GREEN, RST);
    vprintf(fmt, args);
    putchar('\n');
    va_end(args);
}

void ui_section(const char *title) {
    printf("\n  %s%s%s%s\n", BOLD, MAGENTA, title, RST);
}
