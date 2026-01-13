#include <gtest/gtest.h>
#include "mocks/mock_spank_utils.h"
#include <map>
#include <string>

// Re-declare external C functions from the plugin that we want to call
extern "C" {
int slurm_spank_init(spank_t spank_ctxt, int argc, char* argv[]);
int slurm_spank_job_prolog(spank_t spank_ctxt, int argc, char** argv);
}

class MaestroPluginTest : public ::testing::Test {
   protected:
    void SetUp() override { mock_spank_reset(); }
};

TEST_F(MaestroPluginTest, ParsesNrQubitsArg) {
    mock_spank_set_context(S_CTX_REMOTE);
    mock_spank_set_remote(1);

    char* argv[] = {(char*)"nrqubits=15", NULL};
    int argc = 1;

    slurm_spank_job_prolog(nullptr, argc, argv);

    EXPECT_EQ(g_env_vars["maestro_nrqubits"], "15");
}

TEST_F(MaestroPluginTest, RespectsMaxQubitsLimit) {
    mock_spank_set_context(S_CTX_REMOTE);
    mock_spank_set_remote(1);

    char* argv[] = {(char*)"max_qubits=5", (char*)"nrqubits=10", NULL};
    int argc = 2;

    slurm_spank_job_prolog(nullptr, argc, argv);

    EXPECT_EQ(g_env_vars["maestro_nrqubits"], "5");
}

TEST_F(MaestroPluginTest, RespectsMinQubitsLimit) {
    mock_spank_set_context(S_CTX_REMOTE);
    mock_spank_set_remote(1);

    char* argv[] = {(char*)"min_qubits=4", (char*)"nrqubits=2", NULL};
    int argc = 2;

    slurm_spank_job_prolog(nullptr, argc, argv);

    EXPECT_EQ(g_env_vars["maestro_nrqubits"], "4");
}

TEST_F(MaestroPluginTest, ParsesShotsArg) {
    mock_spank_set_context(S_CTX_REMOTE);
    mock_spank_set_remote(1);

    char* argv[] = {(char*)"shots=1000", NULL};
    int argc = 1;

    slurm_spank_job_prolog(nullptr, argc, argv);

    EXPECT_EQ(g_env_vars["maestro_shots"], "1000");
}

TEST_F(MaestroPluginTest, RespectsMaxShotsLimit) {
    mock_spank_set_context(S_CTX_REMOTE);
    mock_spank_set_remote(1);

    char* argv[] = {(char*)"max_shots=500", (char*)"shots=1000", NULL};
    int argc = 2;

    slurm_spank_job_prolog(nullptr, argc, argv);

    EXPECT_EQ(g_env_vars["maestro_shots"], "500");
}

TEST_F(MaestroPluginTest, ParsesMaxBondDimArg) {
    mock_spank_set_context(S_CTX_REMOTE);
    mock_spank_set_remote(1);

    char* argv[] = {(char*)"max_bond_dim=64", NULL};
    int argc = 1;

    slurm_spank_job_prolog(nullptr, argc, argv);

    EXPECT_EQ(g_env_vars["maestro_max_bond_dim"], "64");
}

TEST_F(MaestroPluginTest, RespectsMaxMbdLimit) {
    mock_spank_set_context(S_CTX_REMOTE);
    mock_spank_set_remote(1);

    char* argv[] = {(char*)"max_mbd=32", (char*)"max_bond_dim=64", NULL};
    int argc = 2;

    slurm_spank_job_prolog(nullptr, argc, argv);

    EXPECT_EQ(g_env_vars["maestro_max_bond_dim"], "32");
}

TEST_F(MaestroPluginTest, DoesNotSetEnvInNonRemoteContext) {
    mock_spank_set_context(S_CTX_LOCAL);
    mock_spank_set_remote(0);

    char* argv[] = {(char*)"nrqubits=10", NULL};
    int argc = 1;

    slurm_spank_job_prolog(nullptr, argc, argv);

    // In local context, it uses setenv() which we don't mock in g_env_vars
    // Our mock spank_setenv only populates g_env_vars if spank_remote() is true.
    // Wait, let's look at mock_spank.cpp implementation.
    /*
    spank_err_t spank_setenv(spank_t spank, const char *var, const char *val, int overwrite) {
        if (overwrite || g_env_vars.find(var) == g_env_vars.end()) {
            g_env_vars[var] = val;
        }
        return ESPANK_SUCCESS;
    }
    */
    // My mock implementation is a bit too simple, it always sets g_env_vars.
    // But the real code does:
    /*
    if (spank_remote(spank_ctxt)) {
        err = spank_setenv(spank_ctxt, "maestro_nrqubits", std::to_string(nrQubits).c_str(), 1);
    } else {
        setenv("maestro_nrqubits", std::to_string(nrQubits).c_str(), 1);
    }
    */
    // So if spank_remote is false, it uses standard setenv.
    // We can verify g_env_vars is empty.
    EXPECT_TRUE(g_env_vars.empty());
}
