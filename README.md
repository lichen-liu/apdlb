# apert
Auto-Parallelization Execution Run Time

[![main-ci-actions-badge](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml/badge.svg)](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml)

## Execution Runtime
See `src/ert`

## Auto Parallelization with Rose Compiler
See `src/ap`

### (Optional) Set up Rose Compiler via Docker

#### Build Docker image
```bash
cd rose_docker
# Takes about 5 hours, or modify the make -j arg in Dockerfile line 67 to speed up
docker build . --platform x86_64 --tag rose_build
```
#### Enter Docker container
```bash
# cd project_root
docker run -v "$(pwd)":/apert -w="/apert" -it rose_build bash
```

### Build `ap`
```bash
# cd project_root

# If ROSE_PATH env is set
make ap
# Or set ROSE_PATH env by yourself
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make ap
```

### Use `ap_exe` to parallelize code
```bash
cd project_root
```

#### Running `ap_exe`
```bash
# If ROSE_PATH env is set
make run_ap ARGS="benchmark/kernels/matvecp_bm.cpp"
# Or set ROSE_PATH env by yourself
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make run_ap ARGS="benchmark/kernels/matvecp_bm.cpp" 
```

#### Running generated code
```bash
make agt
```

### Benchmark regression with `ap_exe`
```bash
cd project_root
```

#### Auto-parallelize
```bash
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make run_ap WDIR="benchmark" ARGS="kernels/sorting_bm.cpp"
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make run_ap WDIR="benchmark" ARGS="kernels/matvecp_bm.cpp"
```

#### Rename generated code
```bash
mv benchmark/apert_gen/rose_*.cpp benchmark/apert_gen/[new_name].cpp
```

#### Run regression
```bash
make agbm
```

## Notes
1. execution runtime - ert
    a. wspdr_worker, wspdr_pool
    b. suap_worker, suap_pool
    c. serial_pool
    d. benchmarking
2. benchmarking kernels - kbm
    - Adapted to avoid external function calls, to bypass side effect analysis
        - This can be fixed by providing annot
3. rose compiler auto parallelization - analysis
    - http://rosecompiler.org/uploads/ROSE-UserManual.pdf, p161
    - Only support c-style source code
    - Adapted from autoPar (rose/projects/autoParallelization)
    - Modifications
        - Fix build issues
        - Remove command line interfaces to avoid crashes in command line parser
        - Code clean up and remove unrelated code
        - Change rules for parallelization, autoPar was originally designed to generate openMP parallelism
            - Disallow reduction and lastprivate types of variable sharing
            - When both inner and outer for loops are found to be parallelizable, only parallelize the outer one
        - Use lambda [=] to capture the scope into an ert task
            - shared: readonly, can be caputed by ref or value
            - private: equivalent to firstprivate (does not need to be captured unless declared outside, can be reduced into firstprivate by init the variable)
                - Fix bug when autoPar incorrectly captures nested normalized loop variables as private
            - firstprivate: needs to be captured by value
            - lastprivate: not allowed
            - reduction: not allowed
4. rose compielr auto parallelization - code generation
    - Text-based code generation
    - Using lambda [=] to capture the iteration scope into an ert task