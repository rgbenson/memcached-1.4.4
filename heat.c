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

#define HEAT_DEBUG 0 //RGB Delete

static void *slab_heat_calc_thread(void *arg);
static pthread_t slab_heat_calc_tid;
static volatile int do_run_slab_heat_calc_thread = 1;

static uint64_t write_delta[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t write_ops1[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t write_ops2[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t *write_ops = write_ops1;
static uint64_t *write_ops_prev = write_ops2;

static uint64_t read_delta[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t read_ops1[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t read_ops2[MAX_NUMBER_OF_SLAB_CLASSES];
static uint64_t *read_ops = read_ops1;
static uint64_t *read_ops_prev = read_ops2;

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

static inline void read_write_ops_swap(void) {
    uint64_t *read_tmp, *write_tmp;
    write_tmp = write_ops_prev;
    write_ops_prev = write_ops;
    write_ops = write_tmp;

    read_tmp = read_ops_prev;
    read_ops_prev = read_ops;
    read_ops = read_tmp;
}

static inline void debug_print_ops(void) {
  int i;
  fprintf(stderr, "write_ops:");
  for (i = 0; i < MAX_NUMBER_OF_SLAB_CLASSES; i++) {
    fprintf(stderr, "%lu ", write_ops[i]);
  }
  fprintf(stderr, "\nread_ops:");
  for (i = 0; i < MAX_NUMBER_OF_SLAB_CLASSES; i++) {
    fprintf(stderr, "%lu ", read_ops[i]);
  }
  fprintf(stderr, "\n");
}

static inline void debug_print_delta(void) {
  int i;
  fprintf(stderr, "write_delta:");
  for (i = 0; i < MAX_NUMBER_OF_SLAB_CLASSES; i++) {
    fprintf(stderr, "%lu ", write_delta[i]);
  }
  fprintf(stderr, "\nread_delta:");
  for (i = 0; i < MAX_NUMBER_OF_SLAB_CLASSES; i++) {
    fprintf(stderr, "%lu ", read_delta[i]);
  }
  fprintf(stderr, "\n");
}

static inline void calc_read_write_delta(void) {
    int sid = 0;
    static int write_iters = 0, read_iters = 0;

    for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
        /*
         * If this is the first iteration then initialize the previous
         * ops arrays as well.
         */
        if (write_ops[sid] != 0 && write_iters++ == 0)
            write_ops_prev[sid] = write_ops[sid];
        if (read_ops[sid] != 0 && read_iters++ == 0)
            read_ops_prev[sid] = read_ops[sid];

        /* Deal with wrap in a naive way for now. */
        if (write_ops[sid] < write_ops_prev[sid])
            write_ops_prev[sid] = write_ops[sid];
        if (read_ops[sid] < read_ops_prev[sid])
            read_ops_prev[sid] = read_ops[sid];

        write_delta[sid] = write_ops[sid] - write_ops_prev[sid];
        read_delta[sid] = read_ops[sid] - read_ops_prev[sid];
    }
}

static void *slab_heat_calc_thread(void *arg) {
    while (do_run_slab_heat_calc_thread) {
       if (HEAT_DEBUG)
          fprintf(stderr, "rgb test\n");

       usleep(1000000);
       
       read_write_ops_swap();
       slab_get_read_write_ops(write_ops, read_ops);
       calc_read_write_delta();

       if (HEAT_DEBUG) {
          debug_print_ops();
          debug_print_delta();
       }

       // RGB Need to add heat calc here!
    }
    fprintf(stderr, "rgb exit test\n");

    return NULL;
}
