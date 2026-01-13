# Maestro Plugin Examples

This directory contains examples of how to use the Maestro SLURM SPANK plugin.

## Batch Scripts

*   `maestro.sh`: Basic template for running a single QASM file.
*   `maestro_gpu.sh`: Example using the GPU simulator.
*   `maestro_tensor.sh`: Example using the Tensor Network simulator.
*   `maestro_stabilizer.sh`: Example using the Stabilizer simulator.

## Running Multiple Circuits

### 1. Simple Directory Runner (`run_directory.sh`)
Use this bash script to loop through a directory of QASM files and execute them one by one. This is useful for small batches or when you want a simple serial flow.

```bash
chmod +x run_directory.sh
./run_directory.sh ./my_qasm_files output_prefix
```

### 2. SLURM Job Arrays (`maestro_array.sh`)
For large-scale simulations, **Job Arrays** are the recommended approach. They allow SLURM to schedule multiple simulations in parallel across different nodes.

The example script `maestro_array.sh` shows how to map an array of files to SLURM tasks.

```bash
sbatch maestro_array.sh
```

## QASM Files

*   `test.qasm`: A basic 3-qubit circuit.
*   `ghz.qasm`: Preparation of a 3-qubit GHZ state.
*   `teleport.qasm`: Quantum teleportation protocol.
*   `qft.qasm`: 3-qubit Quantum Fourier Transform.
