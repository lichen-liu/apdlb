.ONESHELL: # Applies to every targets in the file!
.SHELLFLAGS += -e

# Default target executed when no arguments are given to make.
default_target: all
.PHONY: default_target

clean:
	rm -rf build/
.PHONY: clean

prepare:
	mkdir -p build
	cd build && cmake ${CMAKE_ARGS} ../

	@echo [=== cmake is successfully prepared ===]
	@echo 
.PHONY: prepare

# tp

tp: prepare
	$(MAKE) -C build tp_tests
	$(MAKE) -C build test ARGS="-R '^tp_.*_tests'"
	@echo [=== tp is successfully tested ===]
	@echo 
.PHONY: tp

# ap
ifdef ROSE_PATH # ROSE Compiler exists
ap: prepare
	$(MAKE) -C build ap_all
	@echo [=== ap is successfully built ===]
	@echo 
.PHONY: ap

run_ap: ap
	./build/ap/ap_exe ${ARGS}
.PHONY: run_ap
else # ROSE Compiler does not exist
ap: prepare
	@echo [=== ap is not supported ===]
	@echo 
.PHONY: ap

run_ap: ap
.PHONY: run_ap
endif

# kbm
kbm: prepare
	$(MAKE) -C build kernels_bm
	$(MAKE) -C build test ARGS="-R '^kernels_.*_bm'"
	@echo [=== kernels is successfully tested ===]
	@echo 
.PHONY: kbm

# all

all: ap tp kbm
.PHONY: all