#include <slurm/slurm.h>
#include <slurm/spank.h>

#include "Simulator.hpp"


SPANK_PLUGIN(maestro_spank, 1)


//extern "C" {

    // local - srun - call 1
    // allocator - sbatch, salloc, scrontab - call 1
    // slurmd - compute - call 1
    // remote - compute (stepd) - call 1 extern, call 1 
    int slurm_spank_init(spank_t spank_ctxt, int argc, char* argv[])
    {
        struct spank_option* opts_to_register = NULL;

        switch (spank_context()) {
            /* salloc, sbatch */
        case S_CTX_ALLOCATOR:
            /* srun */
        case S_CTX_LOCAL:
            /* slurmstepd */
        case S_CTX_REMOTE:
            //opts_to_register = ;
            break;

        default:
            break;
        }
        if (opts_to_register) {
            while (opts_to_register->name) {
                if (spank_option_register(spank_ctxt, opts_to_register++) != ESPANK_SUCCESS) {
                    break;
                }
            }
        }

        return SLURM_SUCCESS;
    }

    // job script - compute - call 1
    int slurm_spank_job_prolog(spank_t spank_ctxt, int argc, char** argv)
    {
        return SLURM_SUCCESS;
    }

    // local - srun - call 2
    // allocator - sbatch, salloc, scrontab - call 2
	// remote - compute (stepd) - call 2 extern, call 2
    int slurm_spank_init_post_opt(spank_t spank_ctxt, int argc, char** argv)
    {
        return SLURM_SUCCESS;
    }

    // local - srun - call 3
    // remote - compute (stepd)
    int slurm_spank_local_user_init(spank_t spank_ctxt, int argc, char** argv)
    {
        return SLURM_SUCCESS;
    }

    // remote - compute (stepd) - call 3
    int slurm_spank_user_init(spank_t spank_ctxt, int argc, char** argv)
    {
        return SLURM_SUCCESS;
    }

    // remote - compute (stepd) - call 4
    int slurm_spank_task_init_privileged(spank_t spank_ctxt, int argc, char** argv)
    {
        return SLURM_SUCCESS;
    }

    // remote - compute (stepd) - call 3 extern, call 5
    int slurm_spank_task_post_fork(spank_t spank_ctxt, int argc, char** argv)
    {
        return SLURM_SUCCESS;
    }

    // remote - compute (stepd) - call 6
    int slurm_spank_task_init(spank_t spank_ctxt, int argc, char** argv)
    {
        if (spank_remote(spank_ctxt)) {
            
        }

        return SLURM_SUCCESS;
    }

    // remote - compute (stepd) - call 7
    int slurm_spank_task_exit(spank_t spank_ctxt, int argc, char** argv)
    {
        int status;

        if (spank_get_item(spank_ctxt, S_TASK_EXIT_STATUS, &status) == ESPANK_SUCCESS) {
        
        }

        return SLURM_SUCCESS;
    }

    // local - srun - call 4
    // allocator - sbatch, salloc, scrontab - call 3
    // remote - compute (stepd) - call 4 extern, call 8
    int slurm_spank_exit(spank_t spank_ctxt, int argc, char** argv)
    {
        return SLURM_SUCCESS;
    }

    // job script - compute - call 2
    int slurm_spank_job_epilog(spank_t spank_ctxt, int argc, char** argv)
    {
        return SLURM_SUCCESS;
    }

    // slurmd - compute - call 2
    int slurm_spank_slurmd_exit(spank_t spank_ctxt, int argc, char** argv)
    {
        return SLURM_SUCCESS;
    }

//}

