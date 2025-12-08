### Maestro Slurm spank plugin

#### Compiling

Create a 'build' directory in the project directory.
Issue a `cmake ..` from it.
From the project directory, issue a `cmake --build build`.
This will also download and build maestro.
Note: Maestro can be built and installed from its own project directory, but the cmake files in this project will take care of downloading and building it.

#### Installing

Copy the maestro executable into a binary directory where it can be found by slurm (for example `/bin`).
Copy the maestro library (`maestro.so`) into a directory where it can be loaded when slurm executed maestro (for example `/lib`)... don't forget to execute `ldconfig` afterwards.
The maestro executable can be executed standalone, too, issue a `maestro -h` to find its command line options.

Copy the plugin (`maestro_spank_plugin.so`) in the slurm plugins directory (for example, `/usr/lib64/slurm`).
In `/etc/slurm/plugstack.conf` add a line for the maestro plugin (minimal: `optional /usr/lib64/slurm/maestro_spank_plugin.so`). See the `examples` directory for an example with arguments.

#### Execution

`maestro` command line options will override the settings from slurm (they are transferred to maestro through environment variables), but it cannot set values bigger than the ones set with slurm options/arguments.
The arguments in `plugstack.conf` will be set if options are not set (either in the slurm batch file or as srun options if run from command line) and if the later ones are specified, the max/min arguments will limit the values that can be set with the options.

For example, `srun --nrqubits 5 maestro test.qasm` will set the number of qubits to 5 if this is not limited in `plugstack.conf`.
With the example `plugstack.conf`, trying a `srun --nrqubits 50 maestro test.qasm` will actually set the number of qubits to 32, the maximum set in the configuration file.
For an example on how to use it with `sbatch`, please check the batch file provided in the examples directory.

