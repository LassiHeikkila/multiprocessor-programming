BIN_DIR := bin
OBJ_DIR := obj
LODEPNG_DIR := ../dependencies/lodepng
CUDA_INC = /usr/local/cuda-12.3/include/
CUDA_LIB_DIR = /usr/local/cuda-12.3/lib64/

.PHONY: build rebuild clean

CC = clang
LD = clang
CFLAGS = -g -O0

# if you want to use AddressSanitizer, uncomment below
#CFLAGS += -fsanitize=address -fno-omit-frame-pointer

LDFLAGS = $(CFLAGS) -L $(CUDA_LIB_DIR) -Wl,-Map,$(BIN_DIR)/main.map
LIBS = -lOpenCL -lm -lc

C_SRC := \
	main.c \
	zncc_operations.c \

C_SRC_COMMON := \
	../src/panic.c \
	../src/image_operations.c \

C_INC := \
	. \
	../inc \
	$(LODEPNG_DIR) \

C_OBJS := $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(C_SRC)))
C_OBJS += $(addprefix $(OBJ_DIR)/,$(patsubst ../src/%.c,%.o,$(C_SRC_COMMON)))
C_OBJS += $(OBJ_DIR)/lodepng.o

build: $(BIN_DIR)/main $(BIN_DIR)/main.map $(BIN_DIR)/main.disassembly

clean:
	@rm -rf $(BIN_DIR) $(OBJ_DIR)

rebuild: clean build

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR)/main.map: $(BIN_DIR)/main

$(BIN_DIR)/main.disassembly: $(BIN_DIR)/main
	objdump -d -S --source-comment="<src>" $< > $@

$(BIN_DIR)/main: $(C_OBJS) | $(BIN_DIR)
	@echo "Linking final executable"
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