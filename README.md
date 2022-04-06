# apert
Auto-Parallelization Execution Run Time

[![main-ci-actions-badge](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml/badge.svg)](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml)

## Thread Pool
See `src/tp`

## Auto Parallelization with Rose Compiler

### (Optional) Rose Compiler Installation via Docker
```bash
cd rose_docker
# Takes about 5 hours, or modify the make -j arg in Dockerfile line 67 to speed up
docker build . --platform x86_64 --tag rose_build
```

### Running
(Optional) Enter Docker container
```bash
# cd project_root
docker run -v "$(pwd)":/apert -w="/apert" -it rose_build bash
```

Build
```bash
# If ROSE_PATH env is set
make rc
# Or set ROSE_PATH env by yourself
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make rc
```
