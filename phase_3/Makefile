BIN_DIR := bin
OBJ_DIR := obj
COVERAGE_DIR := coverage
LODEPNG_DIR := ../dependencies/lodepng
MUNIT_DIR := ../dependencies/munit
CUDA_INC = /usr/local/cuda-12.3/include/
CUDA_LIB_DIR = /usr/local/cuda-12.3/lib64/

.PHONY: build rebuild clean test coverage

# coverage doesn't work on my machine with clang due to gcov version incompatibility
CC = clang
LD = clang
CFLAGS = -g -O2 -Wall

CFLAGS_TEST = --coverage

# if you want to use AddressSanitizer, uncomment below and switch to clang
#CFLAGS += -fsanitize=address -fno-omit-frame-pointer

LDFLAGS = $(CFLAGS) -L $(CUDA_LIB_DIR) -Wl,-Map,$(BIN_DIR)/main.map
LIBS = -lOpenCL -lm -lc

C_SRC := \
	zncc_operations.c \
	coord_fifo.c \

C_SRC_MAIN := \
	$(C_SRC) \
	main.c \

C_SRC_COMMON := \
	../src/panic.c \
	../src/image_operations.c \

C_SRC_TEST := \
	$(C_SRC) \
	test_main.c \

C_INC := \
	. \
	../inc \
	$(LODEPNG_DIR) \

C_INC_TEST := \
	$(MUNIT_DIR) \

C_OBJS := $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(C_SRC_MAIN)))
C_OBJS += $(addprefix $(OBJ_DIR)/,$(patsubst ../src/%.c,%.o,$(C_SRC_COMMON)))
C_OBJS += $(OBJ_DIR)/lodepng.o

C_OBJS_TEST := $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(C_SRC_TEST)))
C_OBJS_TEST += $(addprefix $(OBJ_DIR)/,$(patsubst ../src/%.c,%.o,$(C_SRC_COMMON)))
C_OBJS_TEST += $(OBJ_DIR)/lodepng.o
C_OBJS_TEST += $(OBJ_DIR)/munit.o

GCNO_FILES = $(patsubst %.o,%.gcno,$(C_OBJS_TEST))
GCDA_FILES = $($(patsubst %.o,%.gcna,$(C_OBJS_TEST)))

build: $(BIN_DIR)/main $(BIN_DIR)/main.map $(BIN_DIR)/main.disassembly

clean:
	@rm -rf $(BIN_DIR) $(OBJ_DIR)

rebuild: clean build

test: CFLAGS += $(CFLAGS_TEST)
test: C_INC += $(C_INC_TEST)
test: $(BIN_DIR)/test_main
	$(BIN_DIR)/test_main

coverage: CC = gcc
coverage: LD = gcc
coverage: clean | $(OBJ_DIR)
	lcov --directory $(OBJ_DIR) --zerocounters
	$(MAKE) test CC=$(CC) LD=$(LD)
	lcov --directory $(OBJ_DIR) --capture --output-file test.info --ignore-errors version,gcov
	genhtml -o $(COVERAGE_DIR) --exclude "dependencies" test.info

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR)/main.map: $(BIN_DIR)/main

$(BIN_DIR)/main.disassembly: $(BIN_DIR)/main
	objdump -d -S $< > $@ 2>/dev/null

$(BIN_DIR)/main: $(C_OBJS) | $(BIN_DIR)
	@echo "Linking final executable"
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR)/test_main: $(C_OBJS_TEST) | $(BIN_DIR)
	@echo "Linking test executable"
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "Compiling $<"
	$(CC) -I $(CUDA_INC) $(addprefix -I,$(C_INC)) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: ../src/%.c | $(OBJ_DIR)
	@echo "Compiling $<"
	$(CC) -I $(CUDA_INC) $(addprefix -I,$(C_INC)) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/lodepng.o: $(LODEPNG_DIR)/lodepng.c | $(OBJ_DIR)
	@echo "Compiling lodepng.c"
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/munit.o: $(MUNIT_DIR)/munit.c | $(OBJ_DIR)
	@echo "Compiling munit.c"
	$(CC) $(CFLAGS) -c -o $@ $<