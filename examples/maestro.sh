#!/bin/bash
#SBATCH --job-name=maestro_job
#SBATCH --output=slurm-%j.out
#SBATCH --error=slurm-%j.err
#SBATCH --time=00:01:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --mem=4G
#SBATCH --nrqubits=3
#SBATCH --shots=10
#SBATCH --simulator_type=qcsim
#SBATCH --simulation_type=mps
#SBATCH --max_bond_dim=8
srun maestro test.qasm