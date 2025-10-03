#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "config.h"
#include "worker_thread.h"
#include "utils.h"

extern const char *sync_mechanism_str[];
extern const char *data_structure_str[];

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s -r <read_ratio> -s <sync_mechanism> -k <kernel_module_path> -d <data_structure>\n"
            "Options:\n"
            "    -r <read_ratio>       Ratio of read operations (0-100).\n"
            "                          Example: -r 70 means 70%% reads, 30%% writes.\n"
            "    -s <sync_mechanism>   Synchronization mechanism to use.\n"
            "                          Available choices:\n"
            "                            spinlock  Use spinlocks\n"
            "                            rcu       Use RCU\n"
            "    -k <kernel_module_path> Path to the lab2 kernel module.\n"
            "    -d <data_structure>     Data structure to use.\n"
            "                          Available choices:\n"
            "                            linked_list  Use linked list\n"
            "                            hash_table   Use hash table\n"
            "                            rb_tree      Use RB tree\n"
            "                            xarray       Use Xarray\n"
            "\n"
            "Example:\n"
            "    %s -r 90 -s rcu -k /lib/modules/.../lab2.ko -d linked_list\n",
            prog, prog);
}

static int parse_args(int argc, char *argv[], int *read_ratio,
                      enum sync_mechanism *sync, 
                      const char **kernel_module_path, 
                      enum data_structure *data_structure)
{
    bool ratio_set = false;
    bool sync_set = false;
    bool path_set = false;
    bool ds_set = false;
    int opt;

    while ((opt = getopt(argc, argv, "hr:s:k:d:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage(argv[0]);
            return 1; // help requested
        case 'r': {
            char *endptr = NULL;
            long value = strtol(optarg, &endptr, 10);
            if (endptr == optarg || *endptr != '\0') {
                fprintf(stderr, "Invalid read ratio: %s\n", optarg);
                return -1;
            }
            if (value < 0 || value > 100) {
                fprintf(stderr, "Read ratio must be between 0 and 100.\n");
                return -1;
            }
            *read_ratio = (int)value;
            ratio_set = true;
            break;
        }
        case 's':
            if (strcmp(optarg, "spinlock") == 0) {
                *sync = SYNC_SPINLOCK;
            } else if (strcmp(optarg, "rcu") == 0) {
                *sync = SYNC_RCU;
            } else {
                fprintf(stderr, "Unknown sync mechanism: %s\n", optarg);
                return -1;
            }
            sync_set = true;
            break;
        case 'k':
            *kernel_module_path = optarg;
            path_set = true;
            break;
        case 'd':
            if (strcmp(optarg, "linked_list") == 0) {
                *data_structure = DS_LINKED_LIST;
            } else if (strcmp(optarg, "hash_table") == 0) {
                *data_structure = DS_HASH_TABLE;
            } else if (strcmp(optarg, "rb_tree") == 0) {
                *data_structure = DS_RB_TREE;
            } else if (strcmp(optarg, "xarray") == 0) {
                *data_structure = DS_XARRAY;
            } else {
                fprintf(stderr, "Unknown data structure: %s\n", optarg);
                return -1;
            }
            ds_set = true;
            break;
        default:
            print_usage(argv[0]);
            return -1;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Unexpected positional arguments.\n");
        return -1;
    }

    if (!ratio_set || !sync_set || !path_set || !ds_set) {
        fprintf(stderr, "Missing required options.\n");
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

extern void *thread_func(void *arg);

void run(struct run_args *args) {
    pthread_barrier_t bar;
    pthread_barrier_init(&bar, NULL, TEST_THREADS);

    struct thread_func_args thread_args[TEST_THREADS];
    pthread_t threads[TEST_THREADS];
    struct timespec start_ts, end_ts;

    if (clock_gettime(CLOCK_MONOTONIC, &start_ts) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TEST_THREADS; i++) {
        thread_args[i].thread_id = i;
        thread_args[i].read_ratio = args->read_ratio;
        thread_args[i].total_ops = PER_THREAD_OPS;
        thread_args[i].delete_ratio = DELETE_RATIO;
        thread_args[i].bar = &bar;

        if (pthread_create(&threads[i], NULL, thread_func, &thread_args[i]) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < TEST_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end_ts) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    double elapsed = (double)(end_ts.tv_sec - start_ts.tv_sec) +
                     (double)(end_ts.tv_nsec - start_ts.tv_nsec) / 1e9;
    printf("Thread workload completed in %.3f seconds\n", elapsed);
    double total_ops = (double)(TEST_THREADS * PER_THREAD_OPS);
    printf("Throughput: %.2f ops/sec\n", total_ops / elapsed);

    pthread_barrier_destroy(&bar);
}

int main(int argc, char *argv[])
{
    int read_ratio = 0;
    enum sync_mechanism sync = SYNC_SPINLOCK;
    enum data_structure data_structure = DS_LINKED_LIST;
    const char *kernel_module_path = NULL;

    int rc = parse_args(argc, argv, &read_ratio, &sync, &kernel_module_path, &data_structure);
    if (rc > 0) {
        return 0; // help printed
    }
    if (rc < 0) {
        return 1;
    }
    rc = install_mod(kernel_module_path, 
                     sync == SYNC_SPINLOCK ? "spinlock" : "rcu",
                     data_structure_str[data_structure]);
    if (rc != 0) {
        fprintf(stderr, "Failed to install kernel module.\n");
        return 1;
    }

    printf("Parsed options: read_ratio=%d, sync=%s\n", read_ratio,
           sync == SYNC_SPINLOCK ? "spinlock" : "rcu");
    printf("Kernel module path: %s\n", kernel_module_path);

    struct run_args args = {
        .read_ratio = read_ratio,
        .sync = sync,
    };

    run(&args);

    remove_mod("lab2.ko");

    return 0;
}
