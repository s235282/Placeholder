#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/random.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "worker_thread.h"
#include "utils.h"

#define RAND_RANGE 1000 // Generate random number in [0, RAND_RANGE)

static uint64_t entropy64(void) {
    uint64_t value = 0;
    if (getrandom(&value, sizeof(value), GRND_NONBLOCK) == (ssize_t)sizeof(value)) {
        return value;
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        value ^= ((uint64_t)ts.tv_sec << 32) ^ (uint64_t)ts.tv_nsec;
    } else {
        value ^= (uint64_t)time(NULL);
    }

    value ^= (uint64_t)getpid();
    value ^= (uint64_t)(uintptr_t)pthread_self();

    return value ? value : 0x9e3779b97f4a7c15ULL;
}

/* Blend entropy with thread-local data so each stream starts diverged. */
static uint64_t make_seed(const struct thread_func_args *args, uint64_t stream_tag) {
    uint64_t seed = entropy64();
    seed ^= ((uint64_t)(unsigned)args->thread_id << 48);
    seed ^= stream_tag * 0x9e3779b97f4a7c15ULL;
    return seed ? seed : stream_tag ^ 0xd1b54a32d192ed03ULL;
}

void *thread_func(void *arg) {
    int ret;
    struct thread_func_args *args = (struct thread_func_args *)arg;
    uint32_t read_range = args->read_ratio * 10;
    uint32_t del_range = args->delete_ratio * 10;
    char *read_buf;
    char *write_buf;
    // Three i.i.d generators for r/w, add/del and key
    prng_state rng_rw, rng_ad, rng_key;
    prng_seed(&rng_rw, make_seed(args, 0));
    prng_seed(&rng_ad, make_seed(args, 1));
    prng_seed(&rng_key, make_seed(args, 2));

    read_buf = (char *)malloc(READ_BUFSIZE);
    if (!read_buf) {
        perror("Failed to allocate read buffer");
        return NULL;
    }

    write_buf = (char *)malloc(WRITE_BUFSIZE);
    if (!write_buf) {
        perror("Failed to allocate write buffer");
        free(read_buf);
        return NULL;
    }

    // Open procfs file
    FILE *procfs = fopen(PROCFS_PATH, "rw+");
    if (!procfs) {
        perror("Failed to open procfs file");
        free(read_buf);
        free(write_buf);
        return NULL;
    }

    // Wait for all threads to be opened
    pthread_barrier_wait(args->bar);

    // Perform the thread's work here
    for (int i = 0; i < args->total_ops; i++) {
        uint32_t rw_val = prng_uniform(&rng_rw, RAND_RANGE - 1);
        if (rw_val < read_range) {
            // Perform read operation
            ret = fread(read_buf, 1, READ_BUFSIZE, procfs);
            rewind(procfs); // Reset file position to the beginning
            (void)ret; // Ignore return value
        } else {
            // Perform write operation
            uint32_t ad_val = prng_uniform(&rng_ad, RAND_RANGE - 1);
            uint32_t key = (int32_t)prng_uniform(&rng_key, INT_RANGE - 1);
            snprintf(write_buf, WRITE_BUFSIZE, 
                ad_val < del_range ? "d %d\n" : "a %d\n", key);
            rewind(procfs); // Reset file position to the beginning
            if (fwrite(write_buf, 1, strlen(write_buf), procfs) <= 0) {
                perror("Failed to write to procfs file");
            }
        }
    }

    // Close procfs file
    fclose(procfs);
    free(read_buf);
    free(write_buf);

    return NULL;
}
