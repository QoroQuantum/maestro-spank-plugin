#ifndef MOCK_SPANK_UTILS_H
#define MOCK_SPANK_UTILS_H

#include "slurm/spank.h"
#include <vector>
#include <string>
#include <map>

// Helper functions for test setup
void mock_spank_reset();
void mock_spank_set_context(spank_context_t ctx);
void mock_spank_set_remote(int remote);
void mock_spank_set_argv(const std::vector<std::string>& argv);

extern "C" {
// Expose environment map for verification
extern std::map<std::string, std::string> g_env_vars;
}

#endif
