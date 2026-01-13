#include "slurm/spank.h"
#include <stdarg.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>

// Global state for mocks to be controlled by tests
spank_context_t g_current_context = S_CTX_LOCAL;
int g_is_remote = 0;
std::map<std::string, std::string> g_env_vars;
std::vector<std::string> g_job_argv;
uint32_t g_job_stepid = SLURM_BATCH_SCRIPT;

void mock_spank_reset() {
    g_current_context = S_CTX_LOCAL;
    g_is_remote = 0;
    g_env_vars.clear();
    g_job_argv.clear();
    g_job_stepid = SLURM_BATCH_SCRIPT;
}

void mock_spank_set_context(spank_context_t ctx) { g_current_context = ctx; }

void mock_spank_set_remote(int remote) { g_is_remote = remote; }

void mock_spank_set_argv(const std::vector<std::string> &argv) { g_job_argv = argv; }

// SLURM/SPANK API IMPLEMENTATION

extern "C" {

spank_context_t spank_context(void) { return g_current_context; }

int spank_remote(spank_t spank) { return g_is_remote; }

spank_err_t spank_get_item(spank_t spank, int item, ...) {
    va_list args;
    va_start(args, item);

    spank_err_t result = ESPANK_SUCCESS;

    if (item == S_JOB_STEPID) {
        uint32_t *step_ptr = va_arg(args, uint32_t *);
        *step_ptr = g_job_stepid;
    } else if (item == S_JOB_ARGV) {
        int *argc_ptr = va_arg(args, int *);
        char ***argv_ptr = va_arg(args, char ***);

        *argc_ptr = g_job_argv.size();
        // Leaking memory for mock simplicity, or use static buffers
        char **argv_res = new char *[g_job_argv.size() + 1];
        for (size_t i = 0; i < g_job_argv.size(); ++i) {
            argv_res[i] = strdup(g_job_argv[i].c_str());
        }
        argv_res[g_job_argv.size()] = nullptr;
        *argv_ptr = argv_res;
    }
    // Implement others as needed

    va_end(args);
    return result;
}

spank_err_t spank_setenv(spank_t spank, const char *var, const char *val, int overwrite) {
    if (overwrite || g_env_vars.find(var) == g_env_vars.end()) {
        g_env_vars[var] = val;
    }
    return ESPANK_SUCCESS;
}

spank_err_t spank_getenv(spank_t spank, const char *var, char *buf, int len) {
    if (g_env_vars.count(var)) {
        strncpy(buf, g_env_vars[var].c_str(), len);
        return ESPANK_SUCCESS;
    }
    return ESPANK_ENV_NOEXIST;
}

spank_err_t spank_option_register(spank_t spank, struct spank_option *opt) { return ESPANK_SUCCESS; }

spank_err_t spank_option_getopt(spank_t spank, struct spank_option *opt, char **optargp) {
    // Basic mock: always fail or implement lookups if testing option callbacks via this API
    return ESPANK_ERROR;
}

const char *spank_strerror(spank_err_t err) { return "Mock Error"; }
}
