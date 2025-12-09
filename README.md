# Maestro SPANK Plugin

A [SLURM SPANK](https://slurm.schedmd.com/spank.html) (Slurm Plug-in Architecture for Node and Job (K)ontrol) plugin for integrating [Maestro](https://github.com/QoroQuantum/maestro) functionality into SLURM jobs. This plugin allows users to configure Maestro environment simulations directly via SLURM command-line options, such as specifying the number of qubits or shots.

## Prerequisites

To build and use this plugin, you need:

- **CMake** (v3.10 or higher)
- **C++ Compiler** (supporting C++17)
- **SLURM Development Headers** (usually provided by `slurm-devel` or `slurm-dev` packages)
- **Maestro** (The build process will automatically download and build this dependency)

## Building

The project uses CMake for building. It will automatically fetch the Maestro dependency.

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This will generate:
- `maestro_spank_plugin.so`: The shared object file for the SLURM plugin.
- A local build of the `maestro` simulator library.

## Installation

### 1. Install Libraries
You need to install both the Maestro library and the plugin.

**Install Maestro Library:**
Copy the built Maestro library to a location where the linker can find it (e.g., `/usr/local/lib` or `/usr/lib64`).
```bash
# Example (adjust paths as needed)
sudo cp _deps/maestro-build/libmaestro.so /usr/lib64/
sudo ldconfig
```
*Note: Ensure the directory is in your `LD_LIBRARY_PATH` or configured in `/etc/ld.so.conf`.*

**Install the Plugin:**
Copy the plugin shared object to the SLURM plugins directory.
```bash
sudo cp maestro_spank_plugin.so /usr/lib64/slurm/
```

### 2. Configure SLURM
Register the plugin in your SLURM configuration by editing `/etc/slurm/plugstack.conf`. Add the following line:

```conf
optional /usr/lib64/slurm/maestro_spank_plugin.so
```

#### Configuration Arguments
You can pass arguments to the plugin in `plugstack.conf` to set defaults or limits.

```conf
# Example: Set default qubits to 10 and max usage to 32
optional /usr/lib64/slurm/maestro_spank_plugin.so nrqubits=10 max_qubits=32
```

**Available `plugstack.conf` arguments:**
- `nrqubits=<int>`: Default number of qubits.
- `shots=<int>`: Default number of shots.
- `max_bond_dim=<int>`: Default max bond dimension.
- `min_qubits=<int>`: Minimum allowed qubits.
- `max_qubits=<int>`: Maximum allowed qubits (limits user requests).
- `max_shots=<int>`: Maximum allowed shots.
- `max_mbd=<int>`: Maximum allowed bond dimension.

## Usage

When the plugin is loaded, it exposes new command-line options to `srun`, `sbatch`, and `salloc`. These options are converted into environment variables (e.g., `maestro_nrqubits`) that the Maestro application reads.

### Command Line Options

The following new flags are available:

| Flag | Description |
|------|-------------|
| `--nrqubits=<int>` | Number of qubits to simulate. |
| `--shots=<int>` | Number of shots for the execution. |
| `--simulator_type=<type>` | Simulator backend: `auto`, `aer`, `qcsim`, `composite_aer`, `composite_qcsim`, `gpu`. |
| `--simulation_type=<type>` | Simulation method: `auto`, `statevector`, `mps`, `stabilizer`, `tensor`. |
| `--max_bond_dim=<int>` | Maximum bond dimension for MPS simulation. |

### Examples

**Running an interactive job:**
```bash
# Request 5 qubits for the simulation
srun --nrqubits=5 ./my_maestro_app
```

**Using a batch script:**
In a batch script, you can use the `#SBATCH` directives or pass flags to `srun` inside the script.

```bash
#!/bin/bash
#SBATCH --job-name=quantum-job
#SBATCH --nodes=1
#SBATCH --nrqubits=10

srun ./my_maestro_app
```

See the `examples/` directory for complete batch script examples.

## License

This project is licensed under the terms of the [GNU General Public License v3.0](LICENSE).
