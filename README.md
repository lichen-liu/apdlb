# apdlb
Auto-Parallelization with Dynamic Load Balancing

[![main-ci-actions-badge](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml/badge.svg)](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml)

## Thread Pool
See `src/tp`

## Auto Parallelization with Rose Compiler

### (Optional) Rose Compiler Installation via Docker
[ROSE Installation link](https://github.com/rose-compiler/rose/wiki/Install-ROSE-with-Clang-as-frontend#full-version)
```bash
docker pull ouankou/rose:clang-develop
```

### Running
(Optional) Enter Docker container
```bash
# cd project_root
docker run -v "$(pwd)":/apdlb -w="/apdlb" -it ouankou/rose:clang-develop bash
```
Build
```bash
# If ROSE_PATH env is set
make rc
# Or set ROSE_PATH env by yourself
ROSE_PATH=/u/course/ece1754/rose/ROSE_INSTALL make rc
```
