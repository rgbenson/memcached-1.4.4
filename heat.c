/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Slab hash logic
 *
 *       RGB TODO
 *
 * The rest of the file is licensed under the BSD license.  See LICENSE.
 */

#include "memcached.h"
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

static void *slab_heat_calc_thread(void *arg);

static pthread_t slab_heat_calc_tid;
static volatile int do_run_slab_heat_calc_thread = 1;

int start_slab_heat_calc_thread(void) {
    int ret = pthread_create(&slab_heat_calc_tid, NULL,
                             slab_heat_calc_thread, NULL);
    if (ret != 0) {
        fprintf(stderr,
                "Can't create slab heat calc thread: %s\n", strerror(ret));
        return -1;
    }

    return 0;
}

void stop_slab_heat_calc_thread(void) {
    pthread_mutex_lock(&cache_lock);
    do_run_slab_heat_calc_thread = 0;
    pthread_mutex_unlock(&cache_lock);

    /* Wait for the maintenance thread to stop */
    pthread_join(slab_heat_calc_tid, NULL);
}

static void *slab_heat_calc_thread(void *arg) {
    //RGB
    while (do_run_slab_heat_calc_thread) {
       fprintf(stderr, "rgb test\n");
       usleep(1000000);
    }
    fprintf(stderr, "rgb exit test\n");

    return NULL;
}
