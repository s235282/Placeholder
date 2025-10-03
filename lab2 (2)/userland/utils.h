#ifndef LAB2_UTILS_H
#define LAB2_UTILS_H

#include <stdint.h>

typedef struct {
    uint64_t s[4];
} prng_state;

void prng_seed(prng_state *state, uint64_t seed);
uint32_t prng_uniform(prng_state *state, uint32_t range);
int install_mod(const char *kernel_module_path, const char *sync, const char *data_structure);
int remove_mod(const char *module_name);

#endif // LAB2_UTILS_H
