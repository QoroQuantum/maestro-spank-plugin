#!/bin/bash
#SBATCH --job-name=maestro_array
#SBATCH --output=logs/maestro_%A_%a.out
#SBATCH --error=logs/maestro_%A_%a.err
#SBATCH --array=0-2           # Adjust based on the number of files
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --mem=4G
#SBATCH --nrqubits=20         # Plugin options
#SBATCH --shots=1000

# This script demonstrates how to use SLURM Job Arrays to run multiple QASM files in parallel.

# 1. Prepare an array of your QASM files
FILES=(examples/*.qasm)

# 2. Get the specific file for this task index
INPUT_FILE=${FILES[$SLURM_ARRAY_TASK_ID]}

# 3. Create an output filename
OUTPUT_FILE="outputs/$(basename ${INPUT_FILE%.*}).json"

mkdir -p logs outputs

echo "Task $SLURM_ARRAY_TASK_ID: Processing $INPUT_FILE"

# 4. Run maestro using srun (the plugin will handle the --nrqubits etc. automatically)
srun maestro "$INPUT_FILE" "$OUTPUT_FILE"
