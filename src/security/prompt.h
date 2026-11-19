#pragma once

#include "../defs.h"

#define PROMPT_MAX_TRIES 3

typedef enum {
    PROMPT_NO,
    PROMPT_YES,
    PROMPT_INVALID,
} prompt_answer_t;

/* Prints "  <message> [y/N] " to stderr, without a newline. printf-style. */
void print_prompt(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Reads one line from stdin.
   y / yes → PROMPT_YES, n / no / empty line / EOF (Ctrl-D) → PROMPT_NO, anything else → PROMPT_INVALID.
   Case-insensitive, surrounding spaces are ignored. */
prompt_answer_t get_prompt_answer(void);

/* print_prompt + get_prompt_answer. Asks again on an invalid answer, at most PROMPT_MAX_TRIES times.
   Returns true only for an explicit yes. The caller prints what happens next (and logs it). */
bool prompt_confirm(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
