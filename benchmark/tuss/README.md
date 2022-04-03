# Tiny Universe Simulator System (TUSS)


## Algorithm
Paper reference:  
[Fast Simulations of Gravitational Many-body Problem on RV770 GPU](https://arxiv.org/pdf/0904.3659.pdf)


## Executables
```
# Compile all and run test_core
make
```

### cpusim
- CPU SIMulator
- Located in ./src/cpusim
```
# Compile only
make cpusim
# Compile and run
make run_cpusim
# Run with arguments
make run_cpusim ARGS="any_args"
```

### core
- The core common code
- core_lib
- Located in ./src/core
```
# Compile and test
make test_core
# With more verbose output
make test_core ARGS=-V
# See more options available
make test_core ARGS=-h
```

### bicgen
- Bodies Initial Condition GENerator
- Load in TIPSY format, and translate into in-house BIN format
- Located in ./scripts/bicgen
- `python3 -m scripts.bicgen`

### tussgui
- Tiny Universe Simulator System GUI
- Visualiation of trajectories via `SYSTEM_STATE`s
- Input files are expected to be ordered by integer
- Supports Live and Still trajectory mode
- Supports Still snapshot mode
- `python3 -m scripts.tussgui`


## Input/Output Data

### Units
No particular units are expected, but simulation uses 1 as the gravitational constant G.
If converting from realistic data, make sure the transformed numerical values are converted,
such that the gravitational equation with converted values gives identical result, 
as in its original values with gravitational constant that suits the original units, 
but with a gravitational constant G value of 1 instead.

An example conversion scheme:
mass: [in Msun] * G_solar_mass_parsec_kmps(4.3009e-3)
distance: [in ps]
velocity: [in km/s]

### `BODY_STATE`
The state of a single body, consists of `(POS, VEC, MASS)`.  
The serialization format expands each field, so that looks like:
`(POS.x,POS.y,POS.z,VEL.x,VEL.y,VEL.z, MASS)`

### `SYSTEM_STATE`
A collection of `BODY_STATE`, each represents individual `BODY_STATE`.

### `SYSTEM_STATE` in CSV
Print out each `BODY_STATE` in serialization format.  
Each `BODY_STATE` is represented as individual row with strings representing floating values, with comma to separate each field, and ends with a new line.  
This format is not encouraged due to its large file size, and slow deserialization speed due to string processing.

### `SYSTEM_STATE` in BIN
Print out each `BODY_STATE` in binary serialization format.  
Each `BODY_STATE` is represented as a sequence of bytes, with bytes representing floating values.
The specific format looks like the following:
- first 4 bytes: size of floating type (ie., 4 for floating, 8 for double)
- second 4 bytes: number of bodies
- rest: `(POS.x,POS.y,POS.z,VEL.x,VEL.y,VEL.z, MASS)` for each `BODY_STATE`
  
This format is the recommended format.


## Demo

### Visualization
Solar System
```
mkdir -p ./tmp/solar_sys_cpu_log
make run_cpusim ARGS="-i ./data/ic/solar_system.csv -d 0.05 -n10000 -o ./tmp/solar_sys_cpu_log"
python3 -m scripts.tussgui trajectory_still ./tmp/solar_sys_cpu_log
# Or, some animation
python3 -m scripts.tussgui trajectory_live ./tmp/solar_sys_cpu_log
rm -rf ./tmp/solar_sys_cpu_log
```

Galaxy
```
mkdir -p ./tmp/
make run_cpusim ARGS="-i ./data/ic/s0_s112500_g100000_d100000.bin -d 10 -n20 -v -t4 -V1 -o ./tmp --snapshot"
python3 -m scripts.tussgui snapshot ./data/ic/s0_s112500_g100000_d100000.bin
python3 -m scripts.tussgui snapshot ./tmp/s0_s112500_g100000_d100000*
rm -rf ./tmp
```

### Performance
```
make run_cpusim ARGS="-i ./data/ic/solar_system.csv -d 0.05 -n 10000"

# BASIC, 1 thread
make run_cpusim ARGS="-i ./data/ic/benchmark_100000.bin -d 0.001 -n10 -v -t1 -V0"
# BASIC, 4 threads
make run_cpusim ARGS="-i ./data/ic/benchmark_100000.bin -d 0.001 -n10 -v -t4 -V0"
# BASIC, 4 threads, threadpool
make run_cpusim ARGS="-i ./data/ic/benchmark_100000.bin -d 0.001 -n10 -v -t4 --thread_pool -V0"

# SHARED_ACC
make run_cpusim ARGS="-i ./data/ic/benchmark_100000.bin -d 0.001 -n10 -v -t1 -V1"
# SHARED_ACC, 4 threads
make run_cpusim ARGS="-i ./data/ic/benchmark_100000.bin -d 0.001 -n10 -v -t4 -V1"
# SHARED_ACC, 4 threads, threadpool
make run_cpusim ARGS="-i ./data/ic/benchmark_100000.bin -d 0.001 -n10 -v -t4 --thread_pool -V1"

# TP
make run_cpusim ARGS="-i ./data/ic/benchmark_100000.bin -d 0.001 -n10 -v -t1 -V2"
```

```
python3 -m scripts.benchmark cpu
```

### Verification
```
make run_cpusim ARGS="-i ./data/ic/solar_system.csv -d 0.05 -n 500 --verify"
make run_cpusim ARGS="-i ./data/ic/benchmark_100000.bin -d 0.001 -n 2 -t 4 --verify"
```

## CMake and Makefile

### Arg Passing to exe
```
ARGS="<your_args>"
```