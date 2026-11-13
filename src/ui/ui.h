#pragma once

#include "../defs.h"

extern const char *ui_program_name;

void ui_banner(void);

void ui_format_size(uint64_t bytes, char *buf, size_t buf_size);

void ui_tilde_path(const char *path, char *buf, size_t buf_size);

void ui_pad(const char *text, size_t width);

void ui_bar(double fraction, size_t width);

void ui_ok(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void ui_section(const char *title);
