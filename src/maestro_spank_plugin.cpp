extern "C"
{
#include <slurm/slurm.h>
#include <slurm/spank.h>
}

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include "maestro_spank.h"

static int nrQubits = 64;
static int nrShots = 1;
static int simulatorType = 0;
static int simulationType = 0;
static int maxBondDim = 0;

static int _nr_qubits_cb(int val, const char* optarg, int remote) {
	nrQubits = atoi(optarg);

	return SLURM_SUCCESS;
}

static int _nr_shots_cb(int val, const char* optarg, int remote) {
	nrShots = atoi(optarg);

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
		simulatorType = 1000;
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
		simulationType = 1000;
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
	if (spank_remote(spank_ctxt))
	{
		spank_err_t err = spank_setenv(spank_ctxt, "maestro_nrqubits", std::to_string(nrQubits).c_str(), 1);
		if (err != ESPANK_SUCCESS)
		{
			slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
			return err;
		}
		err = spank_setenv(spank_ctxt, "maestro_nrshots", std::to_string(nrShots).c_str(), 1);
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
	}
	else
	{
		setenv("maestro_nrqubits", std::to_string(nrQubits).c_str(), 1);
		setenv("maestro_nrshots", std::to_string(nrShots).c_str(), 1);
		setenv("maestro_simulator_type", std::to_string(simulatorType).c_str(), 1);
		setenv("maestro_simulation_type", std::to_string(simulationType).c_str(), 1);
		setenv("maestro_max_bond_dim", std::to_string(maxBondDim).c_str(), 1);
	}

	return SLURM_SUCCESS;
}

std::string _get_env(spank_t spank_ctxt, const std::string& var_name) {
	static const int str_size = 1024;
	char value[str_size];
	
	if (spank_remote(spank_ctxt))
	{
		spank_err_t err = spank_getenv(spank_ctxt, var_name.c_str(), value, str_size);

		if (err == ESPANK_ENV_NOEXIST)
			return std::string();

		if (err != ESPANK_SUCCESS) {
			slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
			return std::string();
		}
	}
	else {
		char* env_value = getenv(var_name.c_str());
		if (env_value == nullptr)
			return std::string();
		strncpy(value, env_value, str_size);
		value[str_size - 1] = 0;
	}
	
	std::string result(value);

	return result;
}

std::vector<std::string> _get_job_args(spank_t spank_ctxt) {
	std::vector<std::string> job_args;
	
	char** args = nullptr;
	int argc = 0;
	
	spank_err_t err = spank_get_item(spank_ctxt, S_JOB_ARGV, &argc, &args);
	if (err != ESPANK_SUCCESS) {
		slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
		return job_args;
	}
	
	job_args.reserve(argc);
	
	for (int i = 0; i < argc; ++i) {
		job_args.push_back(std::string(args[i]));
	}
	
	return job_args;
}



struct spank_option maestro_spank_options[] = {
	{(char*)"nr_qubits",
	 (char*)"Qubits",
	 (char*)"Number of qubits in the simulator.",
	 1,
	 0,
	 (spank_opt_cb_f)_nr_qubits_cb},

	 {(char*)"nr_shots",
	 (char*)"Shots",
	 (char*)"Number of shots for the execution.",
	 1,
	 0,
	 (spank_opt_cb_f)_nr_shots_cb},

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
	// local - srun - call 1
	// allocator - sbatch, salloc, scrontab - call 1
	// slurmd - compute - call 1
	// remote/slurmstepd context - compute (stepd) - call 1 extern, call 1 
	// from docs: Called just after plugins are loaded. In remote context, this is just after job step is initialized. This function is called before any plugin option processing.
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

	// job script - compute/job_script context - call 1
	// from docs: Called at the same time as the job prolog. If this function returns a non-zero value and the SPANK plugin that contains it is required in the plugstack.conf, the node that this is run on will be drained.
	int slurm_spank_job_prolog(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// local - srun - call 2
	// allocator - sbatch, salloc, scrontab - call 2
	// slurmd context - call 1
	// remote/slurmstepd context - compute (stepd) - call 2 extern, call 2
	// from docs: Called at the same point as slurm_spank_init, but after all user options to the plugin have been processed. 
	// The reason that the init and init_post_opt callbacks are separated is so that plugins can process system-wide options specified in plugstack.conf in the init callback, then process user options, and finally take some action in slurm_spank_init_post_opt if necessary. 
	// In the case of a heterogeneous job, slurm_spank_init is invoked once per job component.
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
	// remote/slurmstepd context - compute (stepd)
	// from docs: Called in local (srun) context only after all options have been processed. This is called after the job ID and step IDs are available. This happens in srun after the allocation is made, but before tasks are launched.
	int slurm_spank_local_user_init(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return _set_env(spank_ctxt);
	}

	// remote/slurmstepd context - compute (stepd) - call 3
	// from docs: Called after privileges are temporarily dropped. (remote context only)
	int slurm_spank_user_init(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return _set_env(spank_ctxt);
	}

	// remote/slurmstepd context - compute (stepd) - call 4
	// from docs: Called for each task just after fork, but before all elevated privileges are dropped. This can run in parallel with slurm_spank_task_post_fork. (remote context only)
	int slurm_spank_task_init_privileged(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// remote/slurmstepd context - compute (stepd) - call 3 extern, call 5
	// from docs: Called for each task from parent process after fork (2) is complete. Due to the fact that slurmd does not exec any tasks until all tasks have completed fork (2), this call is guaranteed to run before the user task is executed. 
	// This can run in parallel with slurm_spank_task_init_privileged. (remote context only)
	int slurm_spank_task_post_fork(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// remote/slurmstepd context - compute (stepd) - call 6
	// from docs: Called for each task just before execve (2). If you are restricting memory with cgroups, memory allocated here will be in the job's cgroup. (remote context only)
	int slurm_spank_task_init(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		if (spank_remote(spank_ctxt)) {

		}

		return SLURM_SUCCESS;
	}

	// remote/slurmstepd context - compute (stepd) - call 7
	// from docs: Called for each task as its exit status is collected by Slurm. (remote context only)
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
	// slurmd context - call 2
	// remote/slurmstepd context - compute (stepd) - call 4 extern, call 8
	// from docs: Called once just before slurmstepd exits in remote context. In local context, called before srun exits.
	int slurm_spank_exit(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// job script - compute/job_script context - call 2
	// from docs: Called at the same time as the job epilog. If this function returns a non-zero value and the SPANK plugin that contains it is required in the plugstack.conf, the node that this is run on will be drained.
	int slurm_spank_job_epilog(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

	// slurmd - compute - call 2
	// from docs: Called in slurmd when the daemon is shut down.
	int slurm_spank_slurmd_exit(spank_t spank_ctxt, int argc, char** argv)
	{
		slurm_debug("%s: argc=%d remote=%d, in %s", maestro_spank, argc, spank_remote(spank_ctxt), __func__);

		return SLURM_SUCCESS;
	}

}



