.ONESHELL: # Applies to every targets in the file!
.SHELLFLAGS += -e

# Default target executed when no arguments are given to make.
default_target: all
.PHONY: default_target

clean:
	rm -rf build/
.PHONY: clean

prepare:
	cmake ${CMAKE_ARGS} -B build
	@echo [=== cmake is successfully prepared ===]
	@echo 
.PHONY: prepare

# tp

tp: prepare
	$(MAKE) -C build tp
	@echo [=== tp is successfully built ===]
	@echo 
.PHONY: tp

# all

all: tp
.PHONY: all