#!/bin/bash
#SBATCH --job-name=maestro_gpu_job
#SBATCH --output=slurm-%j.out
#SBATCH --error=slurm-%j.err
#SBATCH --time=00:05:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --mem=8G
#SBATCH --nrqubits=10
#SBATCH --shots=100
#SBATCH --simulator_type=gpu
#SBATCH --simulation_type=statevector
srun maestro test.qasm
