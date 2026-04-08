CC ?= cc
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
SBDF_OBJ := $(OBJ_DIR)/sbdf_core.pic.o
ARKODE_OBJ := $(OBJ_DIR)/arkode_core.pic.o

SBDF_TEST := $(TEST_BUILD_DIR)/test_sbdf_core
ARKODE_TEST := $(TEST_BUILD_DIR)/test_arkode_core

.PHONY: all libs tests test clean help

all: libs tests

libs: $(SBDF_LIB) $(ARKODE_LIB)

tests: $(SBDF_TEST) $(ARKODE_TEST)

test: tests
	$(SBDF_TEST)
	$(ARKODE_TEST)

$(BUILD_DIR) $(OBJ_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

$(SBDF_OBJ): $(C_DIR)/sbdf_core.c $(C_DIR)/sbdf_core.h | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(PIC_CFLAGS) -c $< -o $@

$(ARKODE_OBJ): $(C_DIR)/arkode_core.c $(C_DIR)/arkode_core.h | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(PIC_CFLAGS) -c $< -o $@

$(SBDF_LIB): $(SBDF_OBJ) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(SHARED_LDFLAGS) $< $(LDLIBS) -o $@

$(ARKODE_LIB): $(ARKODE_OBJ) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(SHARED_LDFLAGS) $< $(LDLIBS) -o $@

$(SBDF_TEST): $(TEST_DIR)/test_sbdf_core.c $(C_DIR)/sbdf_core.c $(C_DIR)/sbdf_core.h | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_sbdf_core.c $(C_DIR)/sbdf_core.c $(LDFLAGS) $(LDLIBS) -o $@

$(ARKODE_TEST): $(TEST_DIR)/test_arkode_core.c $(C_DIR)/arkode_core.c $(C_DIR)/arkode_core.h | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_arkode_core.c $(C_DIR)/arkode_core.c $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)

help:
	@printf '%s\n' \
	  'Targets:' \
	  '  make            Build shared libraries and C test executables' \
	  '  make libs       Build libsbdf_core and libarkode_core for the Python bindings' \
	  '  make tests      Build the C unit-test executables' \
	  '  make test       Build and run the C unit tests' \
	  '  make clean      Remove build artifacts'
