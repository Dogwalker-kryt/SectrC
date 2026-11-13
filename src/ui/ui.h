#pragma once

#include "../defs.h"

/* Name the program was started as (basename of argv[0]), used in hints. */
extern const char *ui_program_name;

/* Big gradient "SECTRC" logo. */
void ui_banner(void);

/* "1.82 TiB", "512 MiB", "37 B" (1024-based units). */
void ui_format_size(uint64_t bytes, char *buf, size_t buf_size);

/* Replaces a leading $HOME with "~". */
void ui_tilde_path(const char *path, char *buf, size_t buf_size);

/* Prints text padded to width visible columns; longer text is cut with "…". UTF-8 aware. */
void ui_pad(const char *text, size_t width);

/* Prints a colored fill bar (green < 70%, yellow < 90%, red) followed by the percentage. */
void ui_bar(double fraction, size_t width);

/* Prints "  ✔ message". */
void ui_ok(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Prints a bold section title such as "OPTIONS". */
void ui_section(const char *title);
