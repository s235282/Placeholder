#include "utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const char *sync_mechanism_str[] = {
    "spinlock",
    "rcu",
};
const char *data_structure_str[] = {
    "linked_list",
    "hash_table",
    "rb_tree",
    "xarray",
};

/*
 * Helper function: Rotates the bits of a 64-bit integer to the left.
 */
static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

/**
 * @brief Seeds a specific generator instance.
 *
 * This must be called on a state object before it is used.
 * It uses splitmix64 to initialize the xoshiro state from a single 64-bit seed.
 *
 * @param state A pointer to the generator state to be initialized.
 * @param seed  A unique 64-bit seed.
 */
void prng_seed(prng_state *state, uint64_t seed) {
    uint64_t z = seed + 0x9e3779b97f4a7c15;
    for (int i = 0; i < 4; ++i) {
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        state->s[i] = z ^ (z >> 31);
    }
}

/**
 * @brief Generates the next 64-bit random number from a specific generator instance.
 *
 * @param state A pointer to an initialized generator state.
 * @return A 64-bit pseudo-random number.
 */
static inline uint64_t prng_next(prng_state *state) {
    const uint64_t result = rotl(state->s[1] * 5, 7) * 9;
    const uint64_t t = state->s[1] << 17;

    state->s[2] ^= state->s[0];
    state->s[3] ^= state->s[1];
    state->s[1] ^= state->s[2];
    state->s[0] ^= state->s[3];

    state->s[2] ^= t;
    state->s[3] = rotl(state->s[3], 45);

    return result;
}

/**
 * @brief Generates a random number in [0, range] (inclusive) from a specific generator.
 *
 * This function is fast and avoids the bias of the modulo operator.
 *
 * @param state A pointer to an initialized generator state.
 * @param range The upper bound of the desired range.
 * @return A random uint32_t between 0 and range.
 */
uint32_t prng_uniform(prng_state *state, uint32_t range) {
    uint64_t m = (uint64_t)range + 1;
    // Get a 32-bit random value by taking the lower half of a 64-bit value
    uint64_t rand_val = prng_next(state) & 0xFFFFFFFF;
    uint64_t product = rand_val * m;
    uint32_t l = (uint32_t)product;

    if (l < m) {
        uint32_t t = -m % m;
        while (l < t) {
            rand_val = prng_next(state) & 0xFFFFFFFF;
            product = rand_val * m;
            l = (uint32_t)product;
        }
    }
    return product >> 32;
}

static int wait_for_child(pid_t pid) {
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    errno = ECHILD;
    return -1;
}

int install_mod(const char *kernel_module_path, const char *sync, const char *data_structure) {
    if (!kernel_module_path || !sync || !data_structure) {
        errno = EINVAL;
        return -1;
    }

    const size_t sync_len = strlen(sync);
    if (sync_len == 0) {
        errno = EINVAL;
        return -1;
    }

    const size_t sync_arg_len = strlen("sync=") + sync_len + 1;
    char *sync_arg = malloc(sync_arg_len);
    if (!sync_arg) {
        return -1;
    }
    snprintf(sync_arg, sync_arg_len, "sync=%s", sync);

    const size_t ds_len = strlen(data_structure);
    if (ds_len == 0) {
        free(sync_arg);
        errno = EINVAL;
        return -1;
    }
    const size_t ds_arg_len = strlen("ds=") + ds_len + 1;
    char *ds_arg = malloc(ds_arg_len);
    if (!ds_arg) {
        free(sync_arg);
        return -1;
    }
    snprintf(ds_arg, ds_arg_len, "ds=%s", data_structure);

    pid_t pid = fork();
    if (pid < 0) {
        free(sync_arg);
        free(ds_arg);
        return -1;
    }
    if (pid == 0) {
        execlp("sudo", "sudo", "insmod", kernel_module_path, sync_arg, ds_arg, (char *)NULL);
        _exit(EXIT_FAILURE);
    }

    free(sync_arg);
    free(ds_arg);
    return wait_for_child(pid);
}

int remove_mod(const char *module_name) {
    if (!module_name || module_name[0] == '\0') {
        errno = EINVAL;
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        execlp("sudo", "sudo", "rmmod", module_name, (char *)NULL);
        _exit(EXIT_FAILURE);
    }

    return wait_for_child(pid);
}
