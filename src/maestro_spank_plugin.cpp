extern "C" {
#include <slurm/slurm.h>
#include <slurm/spank.h>
}

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <limits>
#include <fstream>
#include <regex>

#include "maestro_spank.h"

static int nrQubits = 64;
static bool nrQubitsSet = false;
static int nrShots = 1;
static bool nrShotsSet = false;
static int simulatorType = 0;
static bool simulatorTypeSet = false;
static int simulationType = 0;
static bool simulationTypeSet = false;
static int maxBondDim = 0;
static bool maxBondDimSet = false;
static bool autoSetQubitCount = false;
static bool returnExpectations = false;
static bool returnExpectationsSet = false;

static const char* spank_ctx_names[] = {"S_CTX_ERROR",     "S_CTX_LOCAL",  "S_CTX_REMOTE",
                                        "S_CTX_ALLOCATOR", "S_CTX_SLURMD", "S_CTX_JOB_SCRIPT"};

static const char* _get_spank_ctx_name() {
    int ctx = spank_context();

    if (ctx == S_CTX_ERROR || ctx < 0 || ctx >= static_cast<int>(sizeof(spank_ctx_names) / sizeof(spank_ctx_names[0])))
        return "UNKNOWN";

    return spank_ctx_names[ctx];
}

// #define PRINT_INFO 1

static void _print_info(spank_t spank_ctxt, int argc, char** argv, const char* funcname) {
#ifdef PRINT_INFO
    slurm_info("%s info: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
               _get_spank_ctx_name(), funcname);

    for (int i = 0; i < argc; ++i) slurm_info("%s info: argv[%d] = %s, in %s", maestro_spank, i, argv[i], funcname);

    char** env = nullptr;
    if (spank_get_item(spank_ctxt, S_JOB_ENV, &env) == ESPANK_SUCCESS) {
        if (env) {
            int index = 0;
            while (env[index]) {
                if (strncmp(*env, "SLURM_JOB_NAME", 14) == 0) {
                    char* env_entry = (char*)malloc(strlen(*env) + 1);
                    strcpy(env_entry, *env);
                    char* job_name = strtok(env_entry, "=");
                    job_name = strtok(NULL, "=");
                    slurm_info("%s info: job name: %s", maestro_spank, job_name);
                    free(env_entry);
                }
                slurm_info("%s info: item %s", maestro_spank, env[index]);
                ++index;
            }
        }
    }
#endif
}

static int _parse_qasm_for_qubits(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return 0;

    int total_qubits = 0;
    std::string line;
    // Regex to match qreg name[N];
    std::regex qreg_regex(R"(qreg\s+\w+\s*\[\s*(\d+)\s*\]\s*;)");
    std::smatch match;

    while (std::getline(file, line)) {
        // Skip comments (lines starting with //)
        size_t first = line.find_first_not_of(" \t\n\r");
        if (first != std::string::npos && line.size() > first + 1 && line[first] == '/' && line[first + 1] == '/')
            continue;

        if (line.find("qreg") != std::string::npos) {
            auto search_start = line.cbegin();
            while (std::regex_search(search_start, line.cend(), match, qreg_regex)) {
                if (match.size() > 1) {
                    total_qubits += std::stoi(match[1].str());
                }
                search_start = match.suffix().first;
            }
        }
    }
    return total_qubits;
}

std::string _get_env(spank_t spank_ctxt, const std::string& var_name) {
    static const int str_size = 1024;
    char value[str_size];

    if (spank_remote(spank_ctxt)) {
        spank_err_t err = spank_getenv(spank_ctxt, var_name.c_str(), value, str_size);

        if (err == ESPANK_ENV_NOEXIST) return std::string();

        if (err != ESPANK_SUCCESS) {
            slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
            return std::string();
        }
    } else {
        char* env_value = getenv(var_name.c_str());
        if (env_value == nullptr) return std::string();
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

static bool _get_args_into_options(spank_t spank_ctxt, int argc, char** argv) {
    bool result = false;
    int min_qubits = 1;
    int max_qubits = 64;
    int max_mbd = std::numeric_limits<int>::max();
    int max_shots = std::numeric_limits<int>::max();

    for (int i = 0; i < argc; ++i) {
#ifdef PRINT_INFO
        slurm_info("%s: Parsing argv[%d] = %s", maestro_spank, i, argv[i]);
#endif

        if (!nrQubitsSet && strncmp("nrqubits=", argv[i], 9) == 0) {
            int nr_qubits = atoi(argv[i] + 9);
            if (nrQubits != nr_qubits) {
#ifdef PRINT_INFO
                slurm_info("%s: Overriding number of qubits from %d to %d in %s", maestro_spank, nrQubits, nr_qubits,
                           __func__);
#endif
                nrQubits = nr_qubits;
                nrQubitsSet = true;
                result = true;
            }
        } else if (!nrShotsSet && strncmp("shots=", argv[i], 6) == 0) {
            int nr_shots = atoi(argv[i] + 6);
            if (nrShots != nr_shots) {
#ifdef PRINT_INFO
                slurm_info("%s: Overriding number of shots from %d to %d in %s", maestro_spank, nrShots, nr_shots,
                           __func__);
#endif
                nrShots = nr_shots;
                nrShotsSet = true;
                result = true;
            }
        } else if (!maxBondDimSet && strncmp("max_bond_dim=", argv[i], 13) == 0) {
            int max_bond_dim = atoi(argv[i] + 13);
            if (maxBondDim != max_bond_dim) {
#ifdef PRINT_INFO
                slurm_info("%s: Overriding max bond dimension from %d to %d in %s", maestro_spank, maxBondDim,
                           max_bond_dim, __func__);
#endif
                maxBondDim = max_bond_dim;
                maxBondDimSet = true;
                result = true;
            }
        } else if (strncmp("min_qubits=", argv[i], 11) == 0) {
            min_qubits = atoi(argv[i] + 11);
        } else if (strncmp("max_qubits=", argv[i], 11) == 0) {
            max_qubits = atoi(argv[i] + 11);
        } else if (strncmp("max_shots=", argv[i], 10) == 0) {
            max_shots = atoi(argv[i] + 10);
        } else if (strncmp("max_mbd=", argv[i], 8) == 0) {
            max_mbd = atoi(argv[i] + 8);
        } else if (strncmp("auto_set_qubit_count=", argv[i], 21) == 0) {
            autoSetQubitCount = (atoi(argv[i] + 21) != 0);
        } else if (!returnExpectationsSet && strncmp("expectations=", argv[i], 13) == 0) {
            returnExpectations = (atoi(argv[i] + 13) != 0);
            returnExpectationsSet = true;
            result = true;
        }
    }

    if (nrQubits < min_qubits) {
#ifdef PRINT_INFO
        slurm_info("%s: Number of qubits %d is less than minimium %d in %s", maestro_spank, nrQubits, min_qubits,
                   __func__);
#endif
        nrQubits = min_qubits;
        nrQubitsSet = true;
        result = true;
    }

    if (nrQubits > max_qubits) {
#ifdef PRINT_INFO
        slurm_info("%s: Number of qubits %d is greater than maximum %d in %s", maestro_spank, max_qubits, max_qubits,
                   __func__);
#endif
        nrQubits = max_qubits;
        nrQubitsSet = true;
        result = true;
    }

    if (nrShots > max_shots) {
#ifdef PRINT_INFO
        slurm_info("%s: Number of shots %d is greater than maximum %d in %s", maestro_spank, nrShots, max_shots,
                   __func__);
#endif
        nrShots = max_shots;
        nrShotsSet = true;
        result = true;
    }

    if (maxBondDim > max_mbd || (maxBondDim == 0 && max_mbd > 0)) {
#ifdef PRINT_INFO
        slurm_info("%s: Max bond dimension %d is greater than maximum %d in %s", maestro_spank, maxBondDim, max_mbd,
                   __func__);
#endif
        maxBondDim = max_mbd;
        maxBondDimSet = true;
        result = true;
    }

    if (autoSetQubitCount && !nrQubitsSet) {
        std::vector<std::string> job_args = _get_job_args(spank_ctxt);
        for (const auto& arg : job_args) {
            if (arg.size() > 5 && arg.substr(arg.size() - 5) == ".qasm") {
                int qasm_qubits = _parse_qasm_for_qubits(arg);
                if (qasm_qubits > 0) {
#ifdef PRINT_INFO
                    slurm_info("%s: Setting number of qubits to %d from QASM file %s", maestro_spank, qasm_qubits,
                               arg.c_str());
#endif
                    nrQubits = qasm_qubits;
                    nrQubitsSet = true;
                    result = true;
                    break;
                }
            }
        }
    }

    return result;
}

static bool _option_set() {
    return nrQubitsSet || nrShotsSet || simulatorTypeSet || simulationTypeSet || maxBondDimSet || returnExpectationsSet;
}

static int _nr_qubits_cb(int val, const char* optarg, int remote) {
    nrQubits = atoi(optarg);
    nrQubitsSet = true;

#ifdef PRINT_INFO
    slurm_info("%s: Nr qubits cb, nr qubits: %d", maestro_spank, nrQubits);
#endif

    return SLURM_SUCCESS;
}

static int _nr_shots_cb(int val, const char* optarg, int remote) {
    nrShots = atoi(optarg);
    nrShotsSet = true;

#ifdef PRINT_INFO
    slurm_info("%s: Nr shots cb, nr shots: %d", maestro_spank, nrShots);
#endif

    return SLURM_SUCCESS;
}

static int _simulator_type_cb(int val, const char* optarg, int remote) {
    std::string sim_type_str(optarg);
    std::transform(sim_type_str.begin(), sim_type_str.end(), sim_type_str.begin(),
                   [](unsigned char chr) { return std::tolower(chr); });

    if (sim_type_str == "aer") {
        simulatorType = 0;
    } else if (sim_type_str == "qcsim") {
        simulatorType = 1;
    } else if (sim_type_str == "composite_aer") {
        simulatorType = 2;
    } else if (sim_type_str == "composite_qcsim") {
        simulatorType = 3;
    } else if (sim_type_str == "gpu") {
        simulatorType = 4;
    } else if (sim_type_str == "auto") {
        simulatorType = 1000;
    } else {
        simulatorType = 0;
    }

    simulatorTypeSet = true;

#ifdef PRINT_INFO
    slurm_info("%s: Simulator type cb, sim type: %d", maestro_spank, simulatorType);
#endif

    return SLURM_SUCCESS;
}

static int _simulation_type_cb(int val, const char* optarg, int remote) {
    std::string sim_type_str(optarg);
    std::transform(sim_type_str.begin(), sim_type_str.end(), sim_type_str.begin(),
                   [](unsigned char chr) { return std::tolower(chr); });

    if (sim_type_str == "statevector") {
        simulationType = 0;
    } else if (sim_type_str == "mps") {
        simulationType = 1;
    } else if (sim_type_str == "stabilizer" || sim_type_str == "clifford") {
        simulationType = 2;
    } else if (sim_type_str == "tensor" || sim_type_str == "tensor_network") {
        simulationType = 3;
    } else if (sim_type_str == "auto") {
        simulationType = 1000;
    } else {
        simulationType = 0;
    }

    simulationTypeSet = true;

#ifdef PRINT_INFO
    slurm_info("%s: Simulation type cb, type: %d", maestro_spank, simulationType);
#endif

    return SLURM_SUCCESS;
}

static int _max_bond_cb(int val, const char* optarg, int remote) {
    maxBondDim = atoi(optarg);

    maxBondDimSet = true;

#ifdef PRINT_INFO
    slurm_info("%s: Max bond dim cb, value: %d", maestro_spank, maxBondDim);
#endif

    return SLURM_SUCCESS;
}

static int _auto_set_qubit_count_cb(int val, const char* optarg, int remote) {
    autoSetQubitCount = true;
    return SLURM_SUCCESS;
}

static int _return_expectations_cb(int val, const char* optarg, int remote) {
    returnExpectations = true;
    returnExpectationsSet = true;
    return SLURM_SUCCESS;
}

static int _set_env(spank_t spank_ctxt) {
    if (spank_remote(spank_ctxt)) {
        spank_err_t err;

        if (nrQubitsSet) {
            err = spank_setenv(spank_ctxt, "maestro_nrqubits", std::to_string(nrQubits).c_str(), 1);
            if (err != ESPANK_SUCCESS) {
                slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
                return err;
            }
        }

        if (nrShotsSet) {
            err = spank_setenv(spank_ctxt, "maestro_shots", std::to_string(nrShots).c_str(), 1);
            if (err != ESPANK_SUCCESS) {
                slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
                return err;
            }
        }

        if (simulatorTypeSet) {
            err = spank_setenv(spank_ctxt, "maestro_simulator_type", std::to_string(simulatorType).c_str(), 1);
            if (err != ESPANK_SUCCESS) {
                slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
                return err;
            }
        }

        if (simulationTypeSet) {
            err = spank_setenv(spank_ctxt, "maestro_simulation_type", std::to_string(simulationType).c_str(), 1);
            if (err != ESPANK_SUCCESS) {
                slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
                return err;
            }
        }

        if (maxBondDimSet) {
            err = spank_setenv(spank_ctxt, "maestro_max_bond_dim", std::to_string(maxBondDim).c_str(), 1);
            if (err != ESPANK_SUCCESS) {
                slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
                return err;
            }
        }

        if (returnExpectationsSet) {
            err =
                spank_setenv(spank_ctxt, "maestro_expectations", std::to_string(returnExpectations ? 1 : 0).c_str(), 1);
            if (err != ESPANK_SUCCESS) {
                slurm_error("%s: %s in %s", maestro_spank, spank_strerror(err), __func__);
                return err;
            }
        }
    } else {
        if (nrQubitsSet) setenv("maestro_nrqubits", std::to_string(nrQubits).c_str(), 1);
        if (nrShotsSet) setenv("maestro_shots", std::to_string(nrShots).c_str(), 1);
        if (simulatorTypeSet) setenv("maestro_simulator_type", std::to_string(simulatorType).c_str(), 1);
        if (simulationTypeSet) setenv("maestro_simulation_type", std::to_string(simulationType).c_str(), 1);
        if (maxBondDimSet) setenv("maestro_max_bond_dim", std::to_string(maxBondDim).c_str(), 1);
        if (returnExpectationsSet)
            setenv("maestro_expectations", std::to_string(returnExpectations ? 1 : 0).c_str(), 1);
    }

    return SLURM_SUCCESS;
}

static struct spank_option maestro_spank_options[] = {
    {(char*)"nrqubits", (char*)"Qubits", (char*)"Number of qubits in the simulator.", 1, 0,
     (spank_opt_cb_f)_nr_qubits_cb},

    {(char*)"shots", (char*)"Shots", (char*)"Number of shots for the execution.", 1, 0, (spank_opt_cb_f)_nr_shots_cb},

    {(char*)"simulator_type", (char*)"Simulator",
     (char*)"Simulator type: auto, aer, qcsim, composite_aer, composite_qcsim or gpu.", 1, 0,
     (spank_opt_cb_f)_simulator_type_cb},

    {(char*)"simulation_type", (char*)"Type", (char*)"Simulation type: auto, statevector, mps, stabilizer, tensor.", 1,
     0, (spank_opt_cb_f)_simulation_type_cb},

    {(char*)"max_bond_dim", (char*)"MaxBondDim", (char*)"Maximum bond dimension for mps.", 1, 0,
     (spank_opt_cb_f)_max_bond_cb},
    {(char*)"auto-set-qubit-count", (char*)"AutoQubits", (char*)"Automatically set qubit count from QASM file.", 0, 0,
     (spank_opt_cb_f)_auto_set_qubit_count_cb},
    {(char*)"expectations", (char*)"Expectations", (char*)"Compute expectation values of observables.", 0, 0,
     (spank_opt_cb_f)_return_expectations_cb},

    SPANK_OPTIONS_TABLE_END};

extern "C" {
// local - srun - call 1
// allocator - sbatch, salloc, scrontab - call 1
// slurmd - compute - call 1
// remote/slurmstepd context - compute (stepd) - call 1 extern, call 1
// from docs: Called just after plugins are loaded. In remote context, this is just after job step is initialized. This
// function is called before any plugin option processing.
int slurm_spank_init(spank_t spank_ctxt, int argc, char* argv[]) {
    struct spank_option* opts_to_register = NULL;

    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

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
// from docs: Called at the same time as the job prolog. If this function returns a non-zero value and the SPANK plugin
// that contains it is required in the plugstack.conf, the node that this is run on will be drained.
int slurm_spank_job_prolog(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    if (!spank_remote(spank_ctxt)) return SLURM_SUCCESS;

    uint32_t stepid = 0;
    if (spank_get_item(spank_ctxt, S_JOB_STEPID, &stepid) == ESPANK_SUCCESS) {
        slurm_debug("%s job stepid %d", maestro_spank, stepid);
        if (stepid != SLURM_BATCH_SCRIPT) return SLURM_SUCCESS;
    }

    for (int i = 0; i < argc; ++i) slurm_debug("%s: argv[%d] = %s, in %s", maestro_spank, i, argv[i], __func__);

    _get_args_into_options(spank_ctxt, argc, argv);

    return _option_set() ? _set_env(spank_ctxt) : SLURM_SUCCESS;
}

// local - srun - call 2
// allocator - sbatch, salloc, scrontab - call 2
// slurmd context - call 1
// remote/slurmstepd context - compute (stepd) - call 2 extern, call 2
// from docs: Called at the same point as slurm_spank_init, but after all user options to the plugin have been
// processed. The reason that the init and init_post_opt callbacks are separated is so that plugins can process
// system-wide options specified in plugstack.conf in the init callback, then process user options, and finally take
// some action in slurm_spank_init_post_opt if necessary. In the case of a heterogeneous job, slurm_spank_init is
// invoked once per job component.
int slurm_spank_init_post_opt(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    if (!spank_remote(spank_ctxt)) return SLURM_SUCCESS;

    uint32_t stepid = 0;
    if (spank_get_item(spank_ctxt, S_JOB_STEPID, &stepid) == ESPANK_SUCCESS) {
        slurm_debug("%s job stepid %d", maestro_spank, stepid);
        if (stepid != SLURM_BATCH_SCRIPT) return SLURM_SUCCESS;
    }

    for (int i = 0; i < argc; ++i) slurm_debug("%s: argv[%d] = %s, in %s", maestro_spank, i, argv[i], __func__);

    _get_args_into_options(spank_ctxt, argc, argv);

    return _option_set() ? _set_env(spank_ctxt) : SLURM_SUCCESS;
}

// local - srun - call 3
// remote/slurmstepd context - compute (stepd)
// from docs: Called in local (srun) context only after all options have been processed. This is called after the job ID
// and step IDs are available. This happens in srun after the allocation is made, but before tasks are launched.
int slurm_spank_local_user_init(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    _get_args_into_options(spank_ctxt, argc, argv);

    return _option_set() ? _set_env(spank_ctxt) : SLURM_SUCCESS;
}

// remote/slurmstepd context - compute (stepd) - call 3
// from docs: Called after privileges are temporarily dropped. (remote context only)
int slurm_spank_user_init(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    _get_args_into_options(spank_ctxt, argc, argv);

    return _option_set() ? _set_env(spank_ctxt) : SLURM_SUCCESS;
}

// remote/slurmstepd context - compute (stepd) - call 4
// from docs: Called for each task just after fork, but before all elevated privileges are dropped. This can run in
// parallel with slurm_spank_task_post_fork. (remote context only)
int slurm_spank_task_init_privileged(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    return SLURM_SUCCESS;
}

// remote/slurmstepd context - compute (stepd) - call 3 extern, call 5
// from docs: Called for each task from parent process after fork (2) is complete. Due to the fact that slurmd does not
// exec any tasks until all tasks have completed fork (2), this call is guaranteed to run before the user task is
// executed. This can run in parallel with slurm_spank_task_init_privileged. (remote context only)
int slurm_spank_task_post_fork(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    return SLURM_SUCCESS;
}

// remote/slurmstepd context - compute (stepd) - call 6
// from docs: Called for each task just before execve (2). If you are restricting memory with cgroups, memory allocated
// here will be in the job's cgroup. (remote context only)
int slurm_spank_task_init(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    if (!spank_remote(spank_ctxt)) return SLURM_SUCCESS;

    char* option = NULL;
    if (spank_option_getopt(spank_ctxt, &maestro_spank_options[0], &option) != ESPANK_SUCCESS) return SLURM_ERROR;

    size_t optlen = strlen(option);
    if (option == nullptr || optlen == 0) return SLURM_SUCCESS;

    _get_args_into_options(spank_ctxt, argc, argv);

    return _option_set() ? _set_env(spank_ctxt) : SLURM_SUCCESS;
}

// remote/slurmstepd context - compute (stepd) - call 7
// from docs: Called for each task as its exit status is collected by Slurm. (remote context only)
int slurm_spank_task_exit(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    return SLURM_SUCCESS;
}

// local - srun - call 4
// allocator - sbatch, salloc, scrontab - call 3
// slurmd context - call 2
// remote/slurmstepd context - compute (stepd) - call 4 extern, call 8
// from docs: Called once just before slurmstepd exits in remote context. In local context, called before srun exits.
int slurm_spank_exit(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    return SLURM_SUCCESS;
}

// job script - compute/job_script context - call 2
// from docs: Called at the same time as the job epilog. If this function returns a non-zero value and the SPANK plugin
// that contains it is required in the plugstack.conf, the node that this is run on will be drained.
int slurm_spank_job_epilog(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    return SLURM_SUCCESS;
}

// slurmd - compute - call 2
// from docs: Called in slurmd when the daemon is shut down.
int slurm_spank_slurmd_exit(spank_t spank_ctxt, int argc, char** argv) {
    slurm_debug("%s: argc=%d, remote=%d, context=%s, in %s", maestro_spank, argc, spank_remote(spank_ctxt),
                _get_spank_ctx_name(), __func__);

    _print_info(spank_ctxt, argc, argv, __func__);

    return SLURM_SUCCESS;
}
}
