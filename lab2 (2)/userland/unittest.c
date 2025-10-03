#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <threshold 0-1000>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *endptr = NULL;
    errno = 0;
    long threshold = strtol(argv[1], &endptr, 10);
    if (errno != 0 || endptr == argv[1] || *endptr != '\0' || threshold < 0 || threshold > 1000) {
        fprintf(stderr, "Threshold must be an integer between 0 and 1000.\n");
        return EXIT_FAILURE;
    }

    prng_state rng;
    prng_seed(&rng, (uint64_t)time(NULL));

    const uint32_t range = 1000;
    const size_t sample_count = 10000;
    size_t below_count = 0;

    for (size_t i = 0; i < sample_count; ++i) {
        uint32_t value = prng_uniform(&rng, range);
        if ((long)value < threshold) {
            ++below_count;
        }
    }
    float target_ratio = (float)threshold / (float)(range);
    float actual_ratio = (float)below_count / (float)(sample_count);

    printf("Generated %zu numbers in [0, %u]. %zu were below %ld.\n",
           sample_count, range, below_count, threshold);
    printf("Target ratio: %.3f, Actual ratio: %.3f\n", target_ratio, actual_ratio);

    return EXIT_SUCCESS;
}
