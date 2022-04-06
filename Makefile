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

# rc
ifdef ROSE_PATH # ROSE Compiler exists
rc: prepare
	$(MAKE) -C build rc_all
	@echo [=== rc is successfully built ===]
	@echo 
.PHONY: rc

run_rc: rc
	./build/rc/rc_exe ${ARGS}
.PHONY: run_rc
else # ROSE Compiler does not exist
rc: prepare
	@echo [=== rc is not supported ===]
	@echo 
.PHONY: rc

run_rc: rc
.PHONY: run_rc
endif

# kbm
kbm: prepare
	$(MAKE) -C build kernels_bm
	$(MAKE) -C build test ARGS="-R '^kernels_.*_bm'"
	@echo [=== kernels is successfully tested ===]
	@echo 
.PHONY: kbm

# all

all: rc tp kbm
.PHONY: all