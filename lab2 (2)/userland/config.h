#ifndef LAB2_CONFIG_H
#define LAB2_CONFIG_H

#include <pthread.h>

#define TEST_THREADS 4
#define PER_THREAD_OPS 10000
#define DELETE_RATIO 50 // 50% deletes 50% adds
#define INT_RANGE 1048576
#define PROCFS_PATH "/proc/lab2"

// 1 MB buf should be sufficient for 10000 ints 
// with 5 DS
#define READ_BUFSIZE 1048576
// 1 char + 1 int
#define WRITE_BUFSIZE 16

enum sync_mechanism {
    SYNC_SPINLOCK,
    SYNC_RCU,
};

enum data_structure {
    DS_LINKED_LIST,
    DS_HASH_TABLE,
    DS_RB_TREE,
    DS_XARRAY,
};

struct run_args {
    int read_ratio;
    enum sync_mechanism sync;
};

struct thread_func_args {
    int thread_id;
    int read_ratio;
    int total_ops;
    int delete_ratio;
    pthread_barrier_t *bar;
};

#endif // LAB2_CONFIG_H