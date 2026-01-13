#!/bin/bash

# Usage: ./run_directory.sh <directory_path> [output_prefix]
# Example: ./run_directory.sh ./qasm_files results

if [ -z "$1" ]; then
    echo "Usage: $0 <directory_path> [output_prefix]"
    exit 1
fi

DIR=$1
PREFIX=${2:-"result"}

if [ ! -d "$DIR" ]; then
    echo "Error: Directory $DIR does not exist."
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p outputs

echo "Running Maestro on all QASM files in $DIR..."

for f in "$DIR"/*.qasm; do
    if [ -f "$f" ]; then
        filename=$(basename -- "$f")
        base="${filename%.*}"
        output="outputs/${PREFIX}_${base}.json"

        echo "Processing $f -> $output"

        # We use srun if we are in a SLURM environment, otherwise just run maestro directly
        if command -v srun &> /dev/null; then
            srun maestro "$f" "$output"
        else
            maestro "$f" "$output"
        fi
    fi
done

echo "Done. Results are in the 'outputs' directory."
