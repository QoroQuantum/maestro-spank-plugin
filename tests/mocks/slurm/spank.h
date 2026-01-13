/* MOCK SPANK HEADER for testing purposes */
#ifndef SLURM_SPANK_H
#define SLURM_SPANK_H

#include "slurm.h"

// Define spank context types as they appear in real spank
typedef enum spank_context {
    S_CTX_ERROR = -1,
    S_CTX_LOCAL = 0,
    S_CTX_REMOTE,
    S_CTX_ALLOCATOR,
    S_CTX_SLURMD,
    S_CTX_JOB_SCRIPT,
} spank_context_t;

typedef enum spank_err { ESPANK_SUCCESS = 0, ESPANK_ERROR = -1, ESPANK_ENV_NOEXIST = 1 } spank_err_t;

typedef enum spank_item { S_JOB_UID, S_JOB_GID, S_JOB_ID, S_JOB_STEPID, S_JOB_ARGV, S_JOB_ENV } spank_item_t;

// Opaque handle
typedef void *spank_t;

// Option callback function pointer type
typedef int (*spank_opt_cb_f)(int val, const char *optarg, int remote);

// SPANK option struct
struct spank_option {
    char *name;
    char *arginfo;
    char *usage;
    int has_arg;
    int val;
    spank_opt_cb_f cb;
};

#define SPANK_OPTIONS_TABLE_END \
    { NULL, NULL, NULL, 0, 0, NULL }

// Defines for registration
#define SPANK_PLUGIN(name, version)

// Mock Functions
#ifdef __cplusplus
extern "C" {
#endif

spank_context_t spank_context(void);
int spank_remote(spank_t spank);
spank_err_t spank_get_item(spank_t spank, int item, ...);
spank_err_t spank_setenv(spank_t spank, const char *var, const char *val, int overwrite);
spank_err_t spank_getenv(spank_t spank, const char *var, char *buf, int len);
spank_err_t spank_option_register(spank_t spank, struct spank_option *opt);
spank_err_t spank_option_getopt(spank_t spank, struct spank_option *opt, char **optargp);
const char *spank_strerror(spank_err_t err);

#ifdef __cplusplus
}
#endif

#endif  // SLURM_SPANK_H
