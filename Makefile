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
BUILD_DIR = build/rtems
INSTALL_PREFIX ?= $(HOME)/install
TEST_DIR = tests
TEST_BUILD_DIR = build/rtems/tests

# Compiler flags
ifeq ($(findstring sparc,$(RTEMS_BSP)),sparc)
TARGET_FLAGS = -mcpu=leon3 -mfix-gr712rc
else ifeq ($(findstring arm,$(RTEMS_BSP)),arm)
TARGET_FLAGS = -mthumb -march=armv7-a -mfpu=neon -mfloat-abi=hard
else
$(error Unknown RTEMS_BSP architecture: $(RTEMS_BSP))
endif

# if not running on simultor, can probably safely remove -DSIMULATOR_LIMITED
# if not using static build, remove -DSTATIC_BUILD
CFLAGS = \
    $(TARGET_FLAGS) \
	-std=c99 \
	-O2 \
	-D_POSIX_C_SOURCE=200809L \
    -DRTEMS_BUILD -DSIMULATOR_LIMITED -DSTATIC_BUILD\
	-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
	-g \
	-I$(RTEMS_BSP_LIB)/include \
    -Ishim \
	-I$(HAMMER_SRC)

# Source files

SRCS = \
    $(HAMMER_SRC)/hammer.c \
    $(HAMMER_SRC)/allocator.c \
    $(HAMMER_SRC)/static_alloc.c \
    $(HAMMER_SRC)/system_allocator.c \
    $(HAMMER_SRC)/sloballoc.c \
    $(HAMMER_SRC)/glue.c \
    $(HAMMER_SRC)/datastructures.c \
    $(HAMMER_SRC)/desugar.c \
    $(HAMMER_SRC)/cfgrammar.c \
    $(HAMMER_SRC)/pprint.c \
    $(HAMMER_SRC)/registry.c \
    $(HAMMER_SRC)/platform.c \
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
    $(HAMMER_SRC)/parsers/end.c \
    $(HAMMER_SRC)/parsers/endianness.c \
    $(HAMMER_SRC)/parsers/epsilon.c \
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

# Test Source
TEST_SRCS = \
	$(TEST_DIR)/test_registry.c \
	$(TEST_DIR)/t_grammar.c \
	$(TEST_DIR)/test_hammer.c \
	$(TEST_DIR)/test_glue.c \
	$(TEST_DIR)/test_datastructures.c \
	$(TEST_DIR)/test_pprint.c \
	$(TEST_DIR)/t_bitreader.c \
	$(TEST_DIR)/t_bitwriter.c \
	$(TEST_DIR)/t_parser.c \
	$(TEST_DIR)/t_misc.c \
	$(TEST_DIR)/test_suite.c \
	$(TEST_DIR)/t_benchmark.c \
	$(TEST_DIR)/t_mm.c \
	$(TEST_DIR)/t_names.c \
	$(TEST_DIR)/t_regression.c \
	$(TEST_DIR)/test_allocator.c \
	$(TEST_DIR)/test_cfgrammar.c \
	$(TEST_DIR)/test_platform.c \
	$(TEST_DIR)/test_sloballoc.c \
	$(TEST_DIR)/test_system_allocator.c \
	$(TEST_DIR)/test_unimplemented.c \
	$(TEST_DIR)/parsers/test_additional.c \
	$(TEST_DIR)/parsers/test_basic.c \
	$(TEST_DIR)/parsers/test_charset.c \
	$(TEST_DIR)/parsers/test_desugar.c \
	$(TEST_DIR)/parsers/test_floats.c \
	$(TEST_DIR)/parsers/test_ignoreseq.c \
	$(TEST_DIR)/parsers/test_indirect.c \
    $(TEST_DIR)/parsers/test_integers.c \
	$(TEST_DIR)/parsers/test_internal.c \
	$(TEST_DIR)/parsers/test_parser_internal.c \
	$(TEST_DIR)/parsers/test_permutation.c \
	$(TEST_DIR)/parsers/test_seek.c \
	$(TEST_DIR)/parsers/test_sequence.c \
	$(TEST_DIR)/parsers/test_token.c \
	$(TEST_DIR)/parsers/test_value.c \
	$(TEST_DIR)/parsers/test_whitespace.c \
	$(TEST_DIR)/parsers/test_xor.c \
	$(TEST_DIR)/backends/test_missing.c \
	$(TEST_DIR)/backends/test_packrat.c

# Object files

OBJS = $(patsubst $(HAMMER_SRC)/%.c, \
		$(BUILD_DIR)/$(subst /,_,%).o, \
		$(SRCS))

TARGET = $(BUILD_DIR)/libhammer.a

TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(TEST_BUILD_DIR)/%.o, $(TEST_SRCS))
TEST_TARGET = $(BUILD_DIR)/libhammer_tests.a

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -I$(TEST_DIR) $(CFLAGS) -c $< -o $@

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
	mkdir -p ~/install/lib ~/install/include
	cp $(TARGET) ~/install/lib/
	cp $(HAMMER_SRC)/hammer.h ~/install/include/
	cp $(HAMMER_SRC)/glue.h ~/install/include/
	cp $(HAMMER_SRC)/compiler_specifics.h ~/install/include/
	cp $(HAMMER_SRC)/allocator.h ~/install/include/
	cp $(HAMMER_SRC)/static_alloc.h ~/install/include/
	cp $(HAMMER_SRC)/rtems_test_suite.h ~/install/include/
	cp $(HAMMER_SRC)/internal.h ~/install/include/
	cp $(HAMMER_SRC)/platform.h ~/install/include/

install-tests: install
	mkdir -p $(INSTALL_PREFIX)/tests
	cp $(TEST_TARGET) $(INSTALL_PREFIX)/tests/
	@echo "Installed tests to $(INSTALL_PREFIX)/tests/"