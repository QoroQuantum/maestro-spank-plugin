#!/bin/bash
#SBATCH --job-name=maestro_stabilizer_job
#SBATCH --output=slurm-%j.out
#SBATCH --error=slurm-%j.err
#SBATCH --time=00:01:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --mem=4G
#SBATCH --nrqubits=30
#SBATCH --shots=1000
#SBATCH --simulator_type=qcsim
#SBATCH --simulation_type=stabilizer
srun maestro test.qasm
