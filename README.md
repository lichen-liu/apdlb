# apdlb
Auto-Parallelization with Dynamic Load Balancing

[![main-ci-actions-badge](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml/badge.svg)](https://github.com/lichen-liu/apdlb/actions/workflows/main_ci.yml)

## Thread Pool
See `src/tp`

## Auto Parallelization with Rose Compiler

### Installation
Install Rose Compiler from Docker: [link](https://github.com/rose-compiler/rose/wiki/Install-ROSE-with-Clang-as-frontend#full-version)
```bash
docker pull ouankou/rose:clang-develop
```

### Running
Enter Docker container
```bash
# cd project_root
docker run -v "$(pwd)":/apdlb -w="/apdlb" -it ouankou/rose:clang-develop bash
```