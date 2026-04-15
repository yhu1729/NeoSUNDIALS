CC ?= cc
PYTHON ?= $(shell if [ -x ./venv/bin/python ]; then printf '%s' ./venv/bin/python; else printf '%s' python3; fi)
UNAME_S := $(shell uname -s 2>/dev/null)

ROOT := .
C_DIR := $(ROOT)/c
TEST_DIR := $(ROOT)/tests
BUILD_DIR := $(ROOT)/build
OBJ_DIR := $(BUILD_DIR)/obj
TEST_BUILD_DIR := $(BUILD_DIR)/c_tests

CSTD ?= -std=c99
OPT ?= -O3
WARN ?= -Wall -Wextra -Wpedantic
CPPFLAGS ?=
CFLAGS ?= $(CSTD) $(OPT) $(WARN)
PIC_CFLAGS ?= $(CFLAGS) -fPIC
LDFLAGS ?=
LDLIBS ?= -lm

ifeq ($(OS),Windows_NT)
  SHARED_EXT := dll
  SHARED_LDFLAGS := -shared
else ifeq ($(UNAME_S),Darwin)
  SHARED_EXT := dylib
  SHARED_LDFLAGS := -dynamiclib
else
  SHARED_EXT := so
  SHARED_LDFLAGS := -shared
endif

SBDF_LIB := $(BUILD_DIR)/libsbdf_core.$(SHARED_EXT)
ARKODE_LIB := $(BUILD_DIR)/libarkode_core.$(SHARED_EXT)
NVECTOR_LIB := $(BUILD_DIR)/libnvector_serial.$(SHARED_EXT)
SBDF_OBJ := $(OBJ_DIR)/sbdf_core.pic.o
ARKODE_OBJ := $(OBJ_DIR)/arkode_core.pic.o
NVECTOR_OBJ := $(OBJ_DIR)/nvector_serial.pic.o

SBDF_TEST := $(TEST_BUILD_DIR)/test_sbdf_core
ARKODE_TEST := $(TEST_BUILD_DIR)/test_arkode_core
NVECTOR_TEST := $(TEST_BUILD_DIR)/test_nvector_serial
ARKODE_ADAPT_TEST := $(TEST_BUILD_DIR)/test_arkode_adapt
ARKODE_INTERP_TEST := $(TEST_BUILD_DIR)/test_arkode_interp
ARKODE_TSTOP_TEST := $(TEST_BUILD_DIR)/test_arkode_tstop
ARKODE_RESET_TEST := $(TEST_BUILD_DIR)/test_arkode_reset
C_TESTS := \
	$(SBDF_TEST) \
	$(ARKODE_TEST) \
	$(NVECTOR_TEST) \
	$(ARKODE_ADAPT_TEST) \
	$(ARKODE_INTERP_TEST) \
	$(ARKODE_TSTOP_TEST) \
	$(ARKODE_RESET_TEST)

.PHONY: all libs tests clean help check check-c check-python

all: libs tests

libs: $(SBDF_LIB) $(ARKODE_LIB) $(NVECTOR_LIB)

tests: $(C_TESTS)

check: check-c check-python
	@echo "All tests passed!"

check-c: tests
	@set -e; \
	for test_exe in $(C_TESTS); do \
	  printf '%s\n' "Running $$test_exe"; \
	  "$$test_exe"; \
	done

check-python: libs
	$(PYTHON) -m unittest discover tests -p test_*.py -v

$(BUILD_DIR) $(OBJ_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

$(SBDF_OBJ): $(C_DIR)/sbdf_core.c $(C_DIR)/sbdf_core.h | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(PIC_CFLAGS) -c $< -o $@

$(ARKODE_OBJ): $(C_DIR)/arkode_core.c $(C_DIR)/arkode_core.h | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(PIC_CFLAGS) -c $< -o $@

$(NVECTOR_OBJ): $(C_DIR)/nvector_serial.c $(C_DIR)/nvector_serial.h | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(PIC_CFLAGS) -c $< -o $@

$(SBDF_LIB): $(SBDF_OBJ) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(SHARED_LDFLAGS) $< $(LDLIBS) -o $@

$(ARKODE_LIB): $(ARKODE_OBJ) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(SHARED_LDFLAGS) $< $(LDLIBS) -o $@

$(NVECTOR_LIB): $(NVECTOR_OBJ) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(SHARED_LDFLAGS) $< $(LDLIBS) -o $@

$(SBDF_TEST): $(TEST_DIR)/test_sbdf_core.c $(C_DIR)/sbdf_core.c $(C_DIR)/sbdf_core.h | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_sbdf_core.c $(C_DIR)/sbdf_core.c $(LDFLAGS) $(LDLIBS) -o $@

$(ARKODE_TEST): $(TEST_DIR)/test_arkode_core.c $(C_DIR)/arkode_core.c $(C_DIR)/arkode_core.h | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_arkode_core.c $(C_DIR)/arkode_core.c $(LDFLAGS) $(LDLIBS) -o $@

$(NVECTOR_TEST): $(TEST_DIR)/test_nvector_serial.c $(C_DIR)/nvector_serial.c $(C_DIR)/nvector_serial.h | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_nvector_serial.c $(C_DIR)/nvector_serial.c $(LDFLAGS) $(LDLIBS) -o $@

$(ARKODE_ADAPT_TEST): $(TEST_DIR)/test_arkode_adapt.c $(C_DIR)/arkode_core.c $(C_DIR)/nvector_serial.c | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_arkode_adapt.c $(C_DIR)/arkode_core.c $(C_DIR)/nvector_serial.c $(LDFLAGS) $(LDLIBS) -o $@

$(ARKODE_INTERP_TEST): $(TEST_DIR)/test_arkode_interp.c $(C_DIR)/arkode_core.c $(C_DIR)/nvector_serial.c | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_arkode_interp.c $(C_DIR)/arkode_core.c $(C_DIR)/nvector_serial.c $(LDFLAGS) $(LDLIBS) -o $@

$(ARKODE_TSTOP_TEST): $(TEST_DIR)/test_arkode_tstop.c $(C_DIR)/arkode_core.c $(C_DIR)/nvector_serial.c | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_arkode_tstop.c $(C_DIR)/arkode_core.c $(C_DIR)/nvector_serial.c $(LDFLAGS) $(LDLIBS) -o $@

$(ARKODE_RESET_TEST): $(TEST_DIR)/test_arkode_reset.c $(C_DIR)/arkode_core.c $(C_DIR)/nvector_serial.c | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_arkode_reset.c $(C_DIR)/arkode_core.c $(C_DIR)/nvector_serial.c $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)

help:
	@printf '%s\n' \
	  'Targets:' \
	  '  make            Build shared libraries and C test executables' \
	  '  make libs       Build libsbdf_core and libarkode_core for the Python bindings' \
	  '  make tests      Build the C unit-test executables' \
	  '  make check-c    Build and run the C unit-test executables' \
	  '  make check-python Build shared libraries and run the Python test suite' \
	  '  make check      Build and run the full C + Python test suite' \
	  '  make clean      Remove build artifacts'
