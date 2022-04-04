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
	$(MAKE) -C build tp
	@echo [=== tp is successfully built ===]
	@echo 
.PHONY: tp

test_tp: tp
	$(MAKE) -C build tp_tests
	$(MAKE) -C build test ARGS="-R '^tp_.*_tests'"
	@echo [=== tp is successfully tested ===]
	@echo 
.PHONY: test_tp

# all

all: test_tp
.PHONY: all