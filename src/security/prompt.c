#include "prompt.h"
#include <strings.h>

#define PROMPT_LINE_SIZE 16

static void vprint_prompt(const char *fmt, va_list args) {
    fputs("  ", stderr);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, " %s[y/N]%s ", GRAY, RST);
    fflush(stderr);
}

void print_prompt(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprint_prompt(fmt, args);
    va_end(args);
}

static char *trim(char *text) {
    while (isspace((unsigned char)*text)) ++text;

    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';

    return text;
}

prompt_answer_t get_prompt_answer(void) {
    char line[PROMPT_LINE_SIZE];

    if (!fgets(line, sizeof(line), stdin)) {
        fputc('\n', stderr);
        return PROMPT_NO;
    }

    if (!strchr(line, '\n') && !feof(stdin)) {
        int32_t ch;
        while ((ch = getchar()) != '\n' && ch != EOF);
        return PROMPT_INVALID;
    }

    const char *answer = trim(line);
    if (answer[0] == '\0') return PROMPT_NO;
    if (strcasecmp(answer, "y") == 0 || strcasecmp(answer, "yes") == 0) return PROMPT_YES;
    if (strcasecmp(answer, "n") == 0 || strcasecmp(answer, "no") == 0) return PROMPT_NO;
    return PROMPT_INVALID;
}

bool prompt_confirm(const char *fmt, ...) {
    fputc('\n', stderr);

    for (uint8_t tries = 0; tries < PROMPT_MAX_TRIES; ++tries) {
        va_list args;
        va_start(args, fmt);
        vprint_prompt(fmt, args);
        va_end(args);

        prompt_answer_t answer = get_prompt_answer();
        if (answer != PROMPT_INVALID) return answer == PROMPT_YES;

        fprintf(stderr, "  %stype y or n%s\n", GRAY, RST);
    }

    // the caller prints "cancelled", so nothing extra here
    return false;
}
