# Usage:
#   make RTEMS_PREFIX=/opt/rcc-1.3.1-gcc RTEMS_BSP=sparc-gaisler-rtems5/gr712rc
# Swap with whatever BSP is needed for hardware.
#
# Output:
#   build/libhammer.a
#	build/libhammer_tests.a

RTEMS_PREFIX ?= /opt/rcc-1.3.1-gcc
RTEMS_BSP ?= sparc-gaisler-rtems5/gr712rc

RTEMS_ARCH := $(firstword $(subst /, ,$(RTEMS_BSP)))

CC = $(RTEMS_PREFIX)/bin/$(RTEMS_ARCH)-gcc
AR = $(RTEMS_PREFIX)/bin/$(RTEMS_ARCH)-ar
RANLIB = $(RTEMS_PREFIX)/bin/$(RTEMS_ARCH)-ranlib

.DEFAULT_GOAL := all

RTEMS_BSP_LIB = $(RTEMS_PREFIX)/$(RTEMS_BSP)/lib
HAMMER_SRC = src
INSTALL_PREFIX ?= $(HOME)/install
TEST_DIR = tests
STATIC_BUILD ?= 0
BUILD_DIR = build/rtems

ifneq ($(STATIC_BUILD),0)
BUILD_DIR := build/rtems/static
endif

TEST_BUILD_DIR = $(BUILD_DIR)/tests

# Compiler flags
ifeq ($(findstring sparc,$(RTEMS_BSP)),sparc)
TARGET_FLAGS = -mcpu=leon3 -mfix-gr712rc
else ifeq ($(findstring arm,$(RTEMS_BSP)),arm)
TARGET_FLAGS = -mthumb -march=armv7-a -mfpu=neon -mfloat-abi=hard
else
$(error Unknown RTEMS_BSP architecture: $(RTEMS_BSP))
endif

# If not running on a simulator, SIMULATOR_LIMITED can be removed.
# Static memory mode is opt-in: use `make STATIC_BUILD=1`.
RTEMS_DEFINES = -DRTEMS_BUILD -DSIMULATOR_LIMITED
ifneq ($(STATIC_BUILD),0)
RTEMS_DEFINES += -DSTATIC_BUILD
endif

CFLAGS = \
    $(TARGET_FLAGS) \
	-std=c99 \
	-O2 \
	-D_POSIX_C_SOURCE=200809L \
    $(RTEMS_DEFINES) \
	-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
	-g \
	-I$(RTEMS_BSP_LIB)/include \
    -Ishim \
	-I$(HAMMER_SRC)

# RTEMS compiles all test sources even when some test registrations are
# intentionally omitted for unsupported features.
TEST_CFLAGS = $(CFLAGS) -Wno-unused-function

# Source files

SRCS = \
    $(HAMMER_SRC)/hammer.c \
    $(HAMMER_SRC)/allocator.c \
    $(HAMMER_SRC)/system_allocator.c \
    $(HAMMER_SRC)/sloballoc.c \
    $(HAMMER_SRC)/glue.c \
    $(HAMMER_SRC)/datastructures.c \
    $(HAMMER_SRC)/desugar.c \
    $(HAMMER_SRC)/cfgrammar.c \
    $(HAMMER_SRC)/pprint.c \
    $(HAMMER_SRC)/registry.c \
    $(HAMMER_SRC)/platform.c \
    $(HAMMER_SRC)/trace.c \
    $(HAMMER_SRC)/bitreader.c \
    $(HAMMER_SRC)/bitwriter.c \
    $(HAMMER_SRC)/rtems_test_suite.c \
    $(HAMMER_SRC)/benchmark.c \
    $(HAMMER_SRC)/backends/packrat.c \
    $(HAMMER_SRC)/backends/params.c \
    $(HAMMER_SRC)/backends/missing.c \
    $(HAMMER_SRC)/parsers/and.c \
    $(HAMMER_SRC)/parsers/action.c \
    $(HAMMER_SRC)/parsers/attr_bool.c \
    $(HAMMER_SRC)/parsers/bind.c \
    $(HAMMER_SRC)/parsers/bits.c \
    $(HAMMER_SRC)/parsers/butnot.c \
    $(HAMMER_SRC)/parsers/bytes.c \
    $(HAMMER_SRC)/parsers/ch.c \
    $(HAMMER_SRC)/parsers/charset.c \
    $(HAMMER_SRC)/parsers/choice.c \
    $(HAMMER_SRC)/parsers/difference.c \
    $(HAMMER_SRC)/parsers/dispatch.c \
    $(HAMMER_SRC)/parsers/end.c \
    $(HAMMER_SRC)/parsers/endianness.c \
    $(HAMMER_SRC)/parsers/epsilon.c \
    $(HAMMER_SRC)/parsers/float.c \
    $(HAMMER_SRC)/parsers/float_range.c \
    $(HAMMER_SRC)/parsers/ignore.c \
    $(HAMMER_SRC)/parsers/ignoreseq.c \
    $(HAMMER_SRC)/parsers/indirect.c \
    $(HAMMER_SRC)/parsers/int_range.c \
    $(HAMMER_SRC)/parsers/many.c \
    $(HAMMER_SRC)/parsers/not.c \
    $(HAMMER_SRC)/parsers/nothing.c \
    $(HAMMER_SRC)/parsers/optional.c \
    $(HAMMER_SRC)/parsers/permutation.c \
    $(HAMMER_SRC)/parsers/seek.c \
    $(HAMMER_SRC)/parsers/sequence.c \
    $(HAMMER_SRC)/parsers/template.c \
    $(HAMMER_SRC)/parsers/token.c \
    $(HAMMER_SRC)/parsers/unimplemented.c \
    $(HAMMER_SRC)/parsers/value.c \
    $(HAMMER_SRC)/parsers/whitespace.c \
    $(HAMMER_SRC)/parsers/xor.c

ifneq ($(STATIC_BUILD),0)
SRCS += \
	$(HAMMER_SRC)/backends/regex_stub.c \
	$(HAMMER_SRC)/static_alloc.c
else
SRCS += $(HAMMER_SRC)/backends/regex.c
endif

# Keep the RTEMS test archive in sync with the native suite, including new
# parser and backend tests, without maintaining a second handwritten list.
RTEMS_HOST_ONLY_TEST_SRCS = \
	$(TEST_DIR)/backends/test_trace.c \
	$(TEST_DIR)/installed_consumer_smoke.c \
	$(TEST_DIR)/test_static_alloc.c
TEST_SRCS := $(filter-out $(RTEMS_HOST_ONLY_TEST_SRCS), \
	$(sort $(shell find $(TEST_DIR) -type f -name '*.c' -print)))

ifneq ($(STATIC_BUILD),0)
TEST_SRCS += $(TEST_DIR)/test_static_alloc.c
endif

# Object files

OBJS = $(patsubst $(HAMMER_SRC)/%.c, \
		$(BUILD_DIR)/$(subst /,_,%).o, \
		$(SRCS))

TARGET = $(BUILD_DIR)/libhammer.a

TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(TEST_BUILD_DIR)/%.o, $(TEST_SRCS))
TEST_TARGET = $(BUILD_DIR)/libhammer_tests.a

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -I$(TEST_DIR) $(TEST_CFLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@
	@echo "Built: $@"

# Targets

.PHONY: all clean info

all: $(BUILD_DIR) $(TARGET)

tests: $(TARGET) $(TEST_TARGET)

# Create directory if doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

#Link all object files into a static library
$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@
	@echo "Built: $@"

# Pattern rule: compile each source file to a flattened object file
$(BUILD_DIR)/%.o: $(HAMMER_SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parsers_%.o: $(HAMMER_SRC)/parsers/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Print toolchain paths
info:
	@echo "RTEMS_PREFIX : $(RTEMS_PREFIX)"
	@echo "RTEMS_BSP    : $(RTEMS_BSP)"
	@echo "RTEMS_ARCH   : $(RTEMS_ARCH)"
	@echo "CC           : $(CC)"
	@echo "AR           : $(AR)"
	@echo "BSP headers  : $(RTEMS_BSP_LIB)/include"

clean:
	rm -rf $(BUILD_DIR)


# INSTALL

install:
	mkdir -p $(INSTALL_PREFIX)/lib $(INSTALL_PREFIX)/include
	cp $(TARGET) $(INSTALL_PREFIX)/lib/
	cp $(HAMMER_SRC)/hammer.h $(INSTALL_PREFIX)/include/
	cp $(HAMMER_SRC)/glue.h $(INSTALL_PREFIX)/include/
	cp $(HAMMER_SRC)/compiler_specifics.h $(INSTALL_PREFIX)/include/
	cp $(HAMMER_SRC)/allocator.h $(INSTALL_PREFIX)/include/
ifneq ($(STATIC_BUILD),0)
	cp $(HAMMER_SRC)/static_alloc.h $(INSTALL_PREFIX)/include/
endif
	cp $(HAMMER_SRC)/rtems_test_suite.h $(INSTALL_PREFIX)/include/
	cp $(HAMMER_SRC)/internal.h $(INSTALL_PREFIX)/include/
	cp $(HAMMER_SRC)/platform.h $(INSTALL_PREFIX)/include/

install-tests: install
	mkdir -p $(INSTALL_PREFIX)/tests
	cp $(TEST_TARGET) $(INSTALL_PREFIX)/tests/
	@echo "Installed tests to $(INSTALL_PREFIX)/tests/"
