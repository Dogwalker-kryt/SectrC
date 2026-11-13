#pragma once

const char *sectr_color(const char *code);

#define RED     sectr_color("\033[31m")
#define GREEN   sectr_color("\033[32m")
#define YELLOW  sectr_color("\033[33m")
#define BLUE    sectr_color("\033[34m")
#define MAGENTA sectr_color("\033[35m")
#define CYAN    sectr_color("\033[36m")
#define UNDER   "\033[4m" //sectr_color("\033[4m")
#define DIM     "\033[2m" // sectr_color("\033[2m")
#define BOLD    "\033[1m" // sectr_color("\033[1m")
#define RST     "\033[0m" //sectr_color("\033[0m")
#define INVERSE sectr_color("\033[7m")
#define GRAY    "\033[90m" // sectr_color("\033[90m")

#define IDK_WHAT_THIS_COLOR_IS sectr_color("\033[1;38;5;99m")

#define BACK    "\033[D"
