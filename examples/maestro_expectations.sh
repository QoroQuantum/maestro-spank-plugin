#!/bin/bash
#SBATCH --job-name=maestro_expectations
#SBATCH --output=slurm-%j.out
#SBATCH --error=slurm-%j.err
#SBATCH --time=00:01:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --mem=4G
#SBATCH --nrqubits=2
#SBATCH --expectations
srun maestro test_expectations.qasm
