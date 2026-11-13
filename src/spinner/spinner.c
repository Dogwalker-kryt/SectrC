#include "spinner.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

static pthread_t spinner_thread;
static atomic_int spinner_running = 0;

static void *spinner_run_animation(void* arg) {
    (void)arg;
    int dots = 0;

    while (atomic_load(&spinner_running)) {
        printf("\rRunning%.*s   ", dots + 1, "...");
        fflush(stdout);

        dots = (dots + 1) % 3;

        usleep(300000);
    }

    return NULL;
}

void spinner_start_pt(void) {
    if (atomic_exchange(&spinner_running, 1)) return;

    if (pthread_create(&spinner_thread, NULL, spinner_run_animation, NULL) != 0) {
        atomic_store(&spinner_running, 0);
    }
}

void spinner_kill_pt() {
    if (!atomic_exchange(&spinner_running, 0)) return;

    pthread_join(spinner_thread, NULL);

    printf("\r                \r");
    fflush(stdout);
}