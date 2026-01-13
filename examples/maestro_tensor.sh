#!/bin/bash
#SBATCH --job-name=maestro_tensor_job
#SBATCH --output=slurm-%j.out
#SBATCH --error=slurm-%j.err
#SBATCH --time=00:10:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --mem=16G
#SBATCH --nrqubits=20
#SBATCH --shots=100
#SBATCH --simulator_type=auto
#SBATCH --simulation_type=tensor
srun maestro test.qasm
