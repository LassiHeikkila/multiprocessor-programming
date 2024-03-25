BIN_DIR := bin
OBJ_DIR := obj
COVERAGE_DIR := coverage
LODEPNG_DIR := ../dependencies/lodepng
MUNIT_DIR := ../dependencies/munit
CUDA_INC = /usr/local/cuda-12.3/include/
CUDA_LIB_DIR = /usr/local/cuda-12.3/lib64/

.PHONY: clean test coverage
.DEFAULT_GOAL = test

C_SRC_TEST := \
	test_main.c

C_SRC_COMMON := \
	../src/panic.c \
	../src/image_operations.c \
	../src/zncc_operations.c \
	../src/coord_fifo.c \
	../src/device_support.c \

C_INC := \
	. \
	../inc \
	$(LODEPNG_DIR) \
	$(MUNIT_DIR) \
	$(CUDA_INC) \

C_OBJS_TEST := $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(C_SRC_TEST)))
C_OBJS_TEST += $(addprefix $(OBJ_DIR)/,$(patsubst ../src/%.c,%.o,$(C_SRC_COMMON)))
C_OBJS_TEST += $(OBJ_DIR)/lodepng.o
C_OBJS_TEST += $(OBJ_DIR)/munit.o

GCNO_FILES = $(patsubst %.o,%.gcno,$(C_OBJS_TEST))
GCDA_FILES = $($(patsubst %.o,%.gcna,$(C_OBJS_TEST)))

vpath %.c . ../src

# coverage doesn't work on my machine with clang due to gcov version incompatibility
CC = gcc
LD = gcc
CFLAGS = -g -O2 -Wall --coverage -DCL_TARGET_OPENCL_VERSION=120

LDFLAGS = $(CFLAGS) -L $(CUDA_LIB_DIR) -Wl,-Map,$(BIN_DIR)/main.map
LIBS = -lOpenCL -lm -lc

test: $(BIN_DIR)/test_main
	$(BIN_DIR)/test_main

coverage: clean | $(OBJ_DIR)
	lcov --directory $(OBJ_DIR) --zerocounters
	$(MAKE) test
	lcov --directory $(OBJ_DIR) --capture --output-file test.info --ignore-errors version,gcov
	genhtml -o $(COVERAGE_DIR) --exclude "dependencies" test.info

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

$(BIN_DIR)/test_main: $(C_OBJS_TEST) | $(BIN_DIR)
	@echo "Linking test executable"
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "Compiling $^"
	$(CC) $(CFLAGS) $(addprefix -I,$(C_INC)) -c -o $@ $^

$(OBJ_DIR)/lodepng.o: $(LODEPNG_DIR)/lodepng.c | $(OBJ_DIR)
	@echo "Compiling lodepng.c"
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/munit.o: $(MUNIT_DIR)/munit.c | $(OBJ_DIR)
	@echo "Compiling munit.c"
	$(CC) $(CFLAGS) -c -o $@ $<
