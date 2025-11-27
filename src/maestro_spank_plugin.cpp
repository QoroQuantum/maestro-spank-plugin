extern "C"
{
#include <slurm/slurm.h>
#include <slurm/spank.h>
}

#include <algorithm>

#include "Simulator.hpp"

#define maestro_spank "maestro_spank"

static int nrQubits = 64;
static int simulatorType = 0;
static int simulationType = 0;
static int maxBondDim = 0;

static int _nr_qubits_val_cb(int val, const char* optarg, int remote) {
	nrQubits = val;

	return SLURM_SUCCESS;
}

static int _nr_qubits_cb(int val, const char* optarg, int remote) {
	nrQubits = atoi(optarg);

	return SLURM_SUCCESS;
}

static int _simulator_type_cb(int val, const char* optarg, int remote) {
	std::string sim_type_str(optarg);
	std::transform(sim_type_str.begin(), sim_type_str.end(), sim_type_str.begin(),
		[](unsigned char chr) { return std::tolower(chr); });

	if (sim_type_str == "aer") {
		simulatorType = 0;
	}
	else if (sim_type_str == "qcsim") {
		simulatorType = 1;
	}
	else if (sim_type_str == "composite_aer") {
		simulatorType = 2;
	}
	else if (sim_type_str == "composite_qcsim") {
		simulatorType = 3;
	}
	else if (sim_type_str == "gpu") {
		simulatorType = 4;
	}
	else if (sim_type_str == "auto") {
		simulatorType = -1;
	}
	else {
		simulatorType = 0;
	}

	return SLURM_SUCCESS;
}

static int _simulation_type_cb(int val, const char* optarg, int remote) {
	std::string sim_type_str(optarg);
	std::transform(sim_type_str.begin(), sim_type_str.end(), sim_type_str.begin(),
		[](unsigned char chr) { return std::tolower(chr); });

	if (sim_type_str == "statevector") {
		simulationType = 0;
	}
	else if (sim_type_str == "mps") {
		simulationType = 1;
	}
	else if (sim_type_str == "stabilizer" || sim_type_str == "clifford") {
		simulationType = 2;
	}
	else if (sim_type_str == "tensor" || sim_type_str == "tensor_network") {
		simulationType = 3;
	}
	else if (sim_type_str == "auto") {
		simulationType = -1;
	}
	else {
		simulationType = 0;
	}

	return SLURM_SUCCESS;
}

static int _max_bond_cb(int val, const char* optarg, int remote) {
	maxBondDim = atoi(optarg);

	return SLURM_SUCCESS;
}

static int _set_env(spank_t spank_ctxt) {
	spank_err_t err = spank_setenv(spank_ctxt, "maestro_nrqubits", std::to_string(nrQubits).c_str(), 1);
	if (err != ESPANK_SUCCESS)
	{
		slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
		return err;
	}
	err = spank_setenv(spank_ctxt, "maestro_simulator_type", std::to_string(simulatorType).c_str(), 1);
	if (err != ESPANK_SUCCESS)
	{
		slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
		return err;
	}
	err = spank_setenv(spank_ctxt, "maestro_simulation_type", std::to_string(simulationType).c_str(), 1);
	if (err != ESPANK_SUCCESS)
	{
		slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
		return err;
	}
	err = spank_setenv(spank_ctxt, "maestro_max_bond_dim", std::to_string(maxBondDim).c_str(), 1);
	if (err != ESPANK_SUCCESS)
	{
		slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
		return err;
	}

	return SLURM_SUCCESS;
}



struct spank_option maestro_spank_options[] = {
	{(char*)"nr_qubits",
	 (char*)"Qubits",
	 (char*)"Number of qubits in the simulator.",
	 0,
	 0,
	 (spank_opt_cb_f)_nr_qubits_val_cb},

	{(char*)"nr_qubits",
	 (char*)"Qubits",
	 (char*)"Number of qubits in the simulator.",
	 1,
	 0,
	 (spank_opt_cb_f)_nr_qubits_cb},

	{(char*)"simulator_type",
	 (char*)"Simulator",
	 (char*)"Simulator type: auto, aer, qcsim, composite_aer, composite_qcsim or gpu.",
	 1,
	 0,
	 (spank_opt_cb_f)_simulator_type_cb},

	{(char*)"simulation_type",
	 (char*)"Type",
	 (char*)"Simulation type: auto, statevector, mps, stabilizer, tensor.",
	 1,
	 0,
	 (spank_opt_cb_f)_simulation_type_cb},

	 {(char*)"max_bond_dim",
	 (char*)"MaxBondDim",
	 (char*)"Maximum bond dimension for mps.",
	 1,
	 0,
	 (spank_opt_cb_f)_max_bond_cb},

	SPANK_OPTIONS_TABLE_END };

extern "C"
{
	SPANK_PLUGIN(maestro_spank, 1)

	// local - srun - call 1
	// allocator - sbatch, salloc, scrontab - call 1
	// slurmd - compute - call 1
	// remote - compute (stepd) - call 1 extern, call 1 
	int slurm_spank_init(spank_t spank_ctxt, int argc, char* argv[])
	{
		struct spank_option* opts_to_register = NULL;

		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		switch (spank_context()) {
			/* salloc, sbatch */
		case S_CTX_ALLOCATOR:
			/* srun */
		case S_CTX_LOCAL:
			/* slurmstepd */
		case S_CTX_REMOTE:
			opts_to_register = maestro_spank_options;
			break;
		default:
			break;
		}

		if (opts_to_register) {
			while (opts_to_register->name) {
				if (spank_option_register(spank_ctxt, opts_to_register++) != ESPANK_SUCCESS) {
					slurm_error("%s: Failed to register %s in %s", maestro_spank, opts_to_register->name, __func__);
					break;
				}
			}
		}

		return SLURM_SUCCESS;
	}

	// job script - compute - call 1
	int slurm_spank_job_prolog(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// local - srun - call 2
	// allocator - sbatch, salloc, scrontab - call 2
	// remote - compute (stepd) - call 2 extern, call 2
	int slurm_spank_init_post_opt(spank_t spank_ctxt, int argc, char** argv)
	{
		uint32_t stepid = 0;

		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		if (!spank_remote(spank_ctxt))
			return SLURM_SUCCESS;

		if (spank_get_item(spank_ctxt, S_JOB_STEPID, &stepid) == ESPANK_SUCCESS) {
			if (stepid != SLURM_BATCH_SCRIPT)
				return SLURM_SUCCESS;
		}

		return _set_env(spank_ctxt);
	}

	// local - srun - call 3
	// remote - compute (stepd)
	int slurm_spank_local_user_init(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return _set_env(spank_ctxt);
	}

	// remote - compute (stepd) - call 3
	int slurm_spank_user_init(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return _set_env(spank_ctxt);
	}

	// remote - compute (stepd) - call 4
	int slurm_spank_task_init_privileged(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// remote - compute (stepd) - call 3 extern, call 5
	int slurm_spank_task_post_fork(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// remote - compute (stepd) - call 6
	int slurm_spank_task_init(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		if (spank_remote(spank_ctxt)) {

		}

		return SLURM_SUCCESS;
	}

	// remote - compute (stepd) - call 7
	int slurm_spank_task_exit(spank_t spank_ctxt, int argc, char** argv)
	{
		int status;

		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		if (spank_get_item(spank_ctxt, S_TASK_EXIT_STATUS, &status) == ESPANK_SUCCESS) {

		}

		return SLURM_SUCCESS;
	}

	// local - srun - call 4
	// allocator - sbatch, salloc, scrontab - call 3
	// remote - compute (stepd) - call 4 extern, call 8
	int slurm_spank_exit(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// job script - compute - call 2
	int slurm_spank_job_epilog(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// slurmd - compute - call 2
	int slurm_spank_slurmd_exit(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

}



