# apert
Auto-Parallelization Execution Run Time

[![main-ci-actions-badge](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml/badge.svg)](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml)

## 1. Execution Runtime
See `src/ert`

## 2. Auto Parallelization with Rose Compiler
See `src/ap`

### 2.1 (Optional) Set up Rose Compiler via Docker

#### 2.1.1 Build Docker image
```bash
cd rose_docker
# Takes about 5 hours, or modify the make -j arg in Dockerfile line 67 to speed up
docker build . --platform x86_64 --tag rose_build
```
#### 2.1.2 Enter Docker container
```bash
# cd project_root
docker run -v "$(pwd)":/apert -w="/apert" -it rose_build bash
```

### 2.2 Build `ap`
```bash
# cd project_root

# If ROSE_PATH env is set
make ap
# Or set ROSE_PATH env by yourself
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make ap
```

### 2.3 `ap_exe` args
```
<program_filename> [-j<num>] [-d]
```

### 2.4 Use `ap_exe` to parallelize code
```bash
cd project_root
```

#### 2.4.1 Running `ap_exe`
```bash
# If ROSE_PATH env is set
make run_ap ARGS="benchmark/kernels/matvecp_bm.cpp"
# Or set ROSE_PATH env by yourself
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make run_ap ARGS="benchmark/kernels/matvecp_bm.cpp" 
```

#### 2.4.2 Running generated code
```bash
make agt
```

### 2.5 Benchmark regression with `ap_exe`
```bash
cd project_root
```

#### 2.5.1 Running `ap_exe`
```bash
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make run_ap WDIR="benchmark" ARGS="kernels/sorting_bm.cpp"
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make run_ap WDIR="benchmark" ARGS="kernels/matvecp_bm.cpp"
```

#### 2.5.2 Rename generated code
```bash
mv benchmark/apert_gen/rose_*.cpp benchmark/apert_gen/[new_name].cpp
```

#### 2.5.3 Running regression
```bash
make agbm
```

### 2.6 Requirements for Target and Generated Program
Target program
1. Only c-style arrays are supported for parallelization
2. Only for-loops are considered for parallelization
3. Code block with function calls cannot be parallelized
Generated Program
1. Should be compiled with following configurations:
```cmakelists
add_compile_options([others] -std=c++17)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)
```

## 3. TODOs
1. Use proper AST mechanism to insert ERT into target code
    - Properly capture non-pointer-type arrays
2. Fix and test the annotation mechanism for loop-analysis
3. Automatically figure out which ERT to use depending on task workload types
4. Proper handle of nested parallelizable regions
   - Nested parallelizable function calls
   - Nested parallelizable for loops

## 4. Notes
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
            - firstprivate: need to be captured by value
            - lastprivate: not allowed
            - reduction: not allowed
4. rose compielr auto parallelization - code generation
    - Text-based code generation
    - Using lambda [=] to capture the iteration scope into an ert task