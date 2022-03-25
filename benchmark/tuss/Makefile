.ONESHELL: # Applies to every targets in the file!
.SHELLFLAGS += -e

# Default target executed when no arguments are given to make.
default_target: all
.PHONY: default_target

clean:
	rm -rf build/
.PHONY: clean

# CMAKE_ARGS="-DENABLE_FFAST_MATH=ON"
prepare:
	cmake ${CMAKE_ARGS} -B build
	@echo [=== cmake is successfully prepared ===]
	@echo 
.PHONY: prepare

# core

test_core: prepare
	$(MAKE) -C build core_tests
	$(MAKE) -C build test ARGS="-R '^core_tests_'"
	@echo [=== core is successfully tested ===]
	@echo 
.PHONY: test_core

# cpusim

cpusim: prepare
	$(MAKE) -C build cpusim_all
	@echo [=== cpusim is successfully built ===]
	@echo 
.PHONY: cpusim

run_cpusim: cpusim
	./build/cpusim/cpusim_exe ${ARGS}
.PHONY: run_cpusim

# all

all: test_core cpusim
.PHONY: all