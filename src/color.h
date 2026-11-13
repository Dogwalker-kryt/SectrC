#pragma once

/* Returns code, or "" when colors are disabled (--no-color). Defined in globals.c. */
const char *sectr_color(const char *code);

#define RED     sectr_color("\033[31m")
#define GREEN   sectr_color("\033[32m")
#define YELLOW  sectr_color("\033[33m")
#define BLUE    sectr_color("\033[34m")
#define MAGENTA sectr_color("\033[35m")
#define CYAN    sectr_color("\033[36m")
#define UNDER   sectr_color("\033[4m")
#define DIM     sectr_color("\033[2m")
#define BOLD    sectr_color("\033[1m")
#define RST     sectr_color("\033[0m")
#define INVERSE sectr_color("\033[7m")

/* Cursor movement, not a color: always emitted. */
#define BACK    "\033[D"
