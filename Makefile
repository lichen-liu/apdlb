.ONESHELL: # Applies to every targets in the file!
.SHELLFLAGS += -e

CWD := $(abspath $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST))))))

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

# ert

ert: prepare
	$(MAKE) -C build ert_tests
	$(MAKE) -C build test ARGS="-R '^ert_.*_tests'"
	@echo [=== ert is successfully tested ===]
	@echo 
.PHONY: ert

# ap
ifdef ROSE_PATH # ROSE Compiler exists
ap: prepare
	$(MAKE) -C build ap_all
	@echo [=== ap is successfully built ===]
	@echo 
.PHONY: ap

ifndef WDIR
WDIR=.
endif
run_ap: ap
	cd ${WDIR}; $(CWD)/build/ap/ap_exe ${ARGS}
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
	$(MAKE) -C build kernels_all
	$(MAKE) -C build test ARGS="-R '^kernels_.*_bm'"
	@echo [=== kernels is successfully tested ===]
	@echo 
.PHONY: kbm

# agbm
agbm: prepare
	$(MAKE) -C build apert_gen_all
	$(MAKE) -C build test ARGS="-R '^apert_gen_.*_bm'"
	@echo [=== apert_gen is successfully tested ===]
	@echo 
.PHONY: agbm

# agt for temporary testing
agt: prepare
	$(MAKE) -C build tmp_apert_gen_all
	$(MAKE) -C build test ARGS="-R '^tmp_apert_gen_.*_bm'"
	@echo [=== tmp_apert_gen is successfully tested ===]
	@echo 
.PHONY: agt

# all

all: ap ert kbm agbm
.PHONY: all