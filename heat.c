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

#define MIN_HEAT_CALC_USLEEP 10000       // 100 per sec
#define DEFAULT_HEAT_CALC_USLEEP 1000000 // 1 per sec

static void *slab_heat_calc_thread(void *arg);
static pthread_t slab_heat_calc_tid;
static volatile int do_run_slab_heat_calc_thread = 1;
static unsigned int usleep_duration = DEFAULT_HEAT_CALC_USLEEP;

uint64_t total_write_delta, total_read_delta;
static uint64_t write_delta[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t read_delta[MAX_NUMBER_OF_SLAB_CLASSES];

static uint64_t write_ops1[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t write_ops2[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t *write_ops = write_ops1;
static uint64_t *write_ops_prev = write_ops2;

static uint64_t read_ops1[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t read_ops2[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t *read_ops = read_ops1;
static uint64_t *read_ops_prev = read_ops2;

static int64_t write_heat1[MAX_NUMBER_OF_SLAB_CLASSES];
static int64_t write_heat2[MAX_NUMBER_OF_SLAB_CLASSES];
static int64_t *write_heat = write_heat1;
static int64_t *write_heat_prev = write_heat2;

static int64_t read_heat1[MAX_NUMBER_OF_SLAB_CLASSES];
static int64_t read_heat2[MAX_NUMBER_OF_SLAB_CLASSES];
static int64_t *read_heat = read_heat1;
static int64_t *read_heat_prev = read_heat2;

static uint64_t write_heat_delta[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t read_heat_delta[MAX_NUMBER_OF_SLAB_CLASSES];


int start_slab_heat_calc_thread(void) {
    int ret = pthread_create(&slab_heat_calc_tid, NULL,
                             slab_heat_calc_thread, NULL);
    if (ret != 0) {
        fprintf(stderr,
                "Can't create slab heat calc thread: %s\n", strerror(ret));
        return -1;
    }

    // RGB Add a new command line arg that users can set to tweak the usleep duration.
    char *env = getenv("MEMCACHED_HEAT_CALC_USLEEP");
    if (env != NULL) {
        usleep_duration = atoi(env);
        if (usleep_duration < MIN_HEAT_CALC_USLEEP ||
           usleep_duration > DEFAULT_HEAT_CALC_USLEEP) {
           usleep_duration = DEFAULT_HEAT_CALC_USLEEP;
        }
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

static inline void swap_read_write_ops(void) {
    uint64_t *read_ops_tmp, *write_ops_tmp;
    write_ops_tmp = write_ops_prev;
    write_ops_prev = write_ops;
    write_ops = write_ops_tmp;

    read_ops_tmp = read_ops_prev;
    read_ops_prev = read_ops;
    read_ops = read_ops_tmp;

    int64_t *read_heat_tmp, *write_heat_tmp;
    write_heat_tmp = write_heat_prev;
    write_heat_prev = write_heat;
    write_heat = write_heat_tmp;

    read_heat_tmp = read_heat_prev;
    read_heat_prev = read_heat;
    read_heat = read_heat_tmp;
}

static inline void calc_read_write_ops(uint64_t *total_write_delta,
                                       uint64_t *total_read_delta) {
    int sid = 0;

    for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
        /*
         * If this is the first iteration then initialize the previous
         * ops arrays as well.
         */
        static int write_iters = 0, read_iters = 0;
        if (write_ops[sid] != 0 && write_iters++ == 0)
            write_ops_prev[sid] = write_ops[sid];
        if (read_ops[sid] != 0 && read_iters++ == 0)
            read_ops_prev[sid] = read_ops[sid];

        /* Deal with wrap in a naive way for now. */
        if (write_ops[sid] < write_ops_prev[sid])
            write_ops_prev[sid] = write_ops[sid];
        if (read_ops[sid] < read_ops_prev[sid])
            read_ops_prev[sid] = read_ops[sid];

        *total_write_delta +=
            write_delta[sid] = write_ops[sid] - write_ops_prev[sid];
        *total_read_delta +=
            read_delta[sid] = read_ops[sid] - read_ops_prev[sid];
    }
}

static inline void calc_read_write_heat(uint64_t total_write_ops,
                                        uint64_t total_read_ops,
                                        uint64_t total_write_delta,
                                        uint64_t total_read_delta) {
    int sid = 0;
    for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
        /*
         * If this is the first iteration then initialize the previous
         * heat arrays as well.
         */
        static int write_iters = 0, read_iters = 0;
        if (write_heat[sid] != 0 && write_iters++ == 0)
            write_heat_prev[sid] = write_heat[sid];
        if (read_heat[sid] != 0 && read_iters++ == 0)
            read_heat_prev[sid] = read_heat[sid];

        if (total_write_delta != 0) {
            write_heat[sid] = (write_delta[sid] / total_write_delta) * 100;
        } else {
            write_heat[sid] = 0;
        }
        write_heat_delta[sid] = write_heat[sid] - write_heat_prev[sid];

        if (total_read_delta != 0) {
            read_heat[sid] = (read_delta[sid] / total_read_delta) * 100;
        } else {
            read_heat[sid] = 0;
        }
        read_heat_delta[sid] = read_heat[sid] - read_heat_prev[sid];
    }
}

static void *slab_heat_calc_thread(void *arg) {
    while (do_run_slab_heat_calc_thread) {
       uint64_t total_write_ops = 0, total_read_ops = 0;
       uint64_t total_write_delta = 0, total_read_delta = 0;

       swap_read_write_ops();

       // RGB TODO!!!
       //    Need to track the number of allocations that failed per slab.
       //    Right now it is a sum of the eviction, expiry, and tailrepair stats.
       //    Best to have a total per-slab stat floating around.

       slab_get_read_write_ops(write_ops, &total_write_ops,
                               read_ops, &total_read_ops);
       calc_read_write_ops(&total_write_delta, &total_read_delta);

       // RGB Not sure we need the total args here.
       calc_read_write_heat(total_write_ops, total_read_ops,
                            total_write_delta, total_read_delta);

       // RGB Need to add "best" per slab eviction targets calc here!

       DEBUG_PRINT_HEARTBEAT("Heat thread is alive.\n");
       DEBUG_PRINT_READ_WRITE(ops);
       DEBUG_PRINT_READ_WRITE(delta);
       DEBUG_PRINT_READ_WRITE(heat);
       DEBUG_PRINT_READ_WRITE_INT(heat_delta);

       usleep(usleep_duration);
    }
    DEBUG_PRINT_HEARTBEAT("Heat thread is exiting.\n");

    return NULL;
}
