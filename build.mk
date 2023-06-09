C_SRC += $(wildcard ./*.c)
ASM_SRC += $(wildcard ./*.asm)
ATT_SRC += $(wildcard ./*.s)

C_OBJS = $(patsubst %.c, %.o, $(C_SRC))
ASM_OBJS = $(patsubst %.asm, %_asm.o, $(ASM_SRC))
ATT_OBJS = $(patsubst %.s, %.o, $(ATT_SRC))

BUILD_C_OBJS = $(patsubst %.o, $(BUILD_DIR)/%.o, $(C_OBJS))
BUILD_ASM_OBJS = $(patsubst %.o, $(BUILD_DIR)/%.o, $(ASM_OBJS))
BUILD_ATT_OBJS = $(patsubst %.o, $(BUILD_DIR)/%.o, $(ATT_OBJS))

BUILD_OBJS := $(BUILD_C_OBJS) $(BUILD_ASM_OBJS) $(BUILD_ATT_OBJS)

$(BUILD_C_OBJS): $(C_SRC) 
	$(CC) $(C_ARGS) -m$(ELF_F) $^ -c
	mkdir -p $(BUILD_DIR)
	mv $(C_OBJS) $(BUILD_DIR)

$(BUILD_ASM_OBJS): $(ASM_SRC)
	$(foreach asmf, $^, nasm -f elf$(ELF_F) $(asmf) -o $(basename $(asmf))_asm.o;) 
	mkdir -p $(BUILD_DIR)
	mv $(ASM_OBJS) $(BUILD_DIR)

$(BUILD_ATT_OBJS): $(ATT_SRC)
	$(foreach asmf, $^, cc -m32 $(asmf) -c ;) 
	mkdir -p $(BUILD_DIR)
	mv $(ATT_OBJS) $(BUILD_DIR)


i386: $(BUILD_OBJS)
i386: ELF_F = 32


