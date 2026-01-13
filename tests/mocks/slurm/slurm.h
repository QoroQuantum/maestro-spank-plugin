/* MOCK SLURM HEADER for testing purposes */
#ifndef SLURM_H
#define SLURM_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SLURM_SUCCESS 0
#define SLURM_ERROR -1
#define SLURM_BATCH_SCRIPT 0xFFFFFFFE  // Mock value

// Logging mocks
#define slurm_info(...) printf(__VA_ARGS__)
#define slurm_debug(...) printf(__VA_ARGS__)
#define slurm_error(...) fprintf(stderr, __VA_ARGS__)

#endif  // SLURM_H
