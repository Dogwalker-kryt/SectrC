#pragma once

void spinner_start_pt(void);

void spinner_kill_pt(void);
/*
#include "running.h"

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

static pthread_t spinner_thread;
static atomic_int spinner_run_animation = 0;

static void* running_animation(void* arg)
{
    int dots = 0;

    while (atomic_load(&running)) {
        printf("\rLoading%.*s   ", dots + 1, "...");
        fflush(stdout);

        dots = (dots + 1) % 3;

        usleep(300000);
    }

    return NULL;
}

void running_start(void)
{
    if (atomic_exchange(&running, 1))
        return;

    if (pthread_create(&running_thread, NULL, running_animation, NULL) != 0) {
        atomic_store(&running, 0);
    }
}

void running_stop(void)
{
    if (!atomic_exchange(&running, 0))
        return;

    pthread_join(running_thread, NULL);

    printf("\r                \r");
    fflush(stdout);
}
*/