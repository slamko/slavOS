include define.mk

ELF_F=$(ARCH)
OBJS = $(shell find ./build -name '*.o')

x86_build:
	+$(MAKE) -C drivers x86
	+$(MAKE) -C lib x86
	+$(MAKE) -C bin x86

x86_64_build:
	+$(MAKE) -C drivers x86_64
	+$(MAKE) -C lib x86_64

kernel.elf: arch/$(ARCH)/start.asm kernel.c
	nasm -f elf$(ELF_F) arch/$(ARCH)/start.asm
	$(CC) -m$(ELF_F) $(C_ARGS) kernel.c 
	ld $(LD_ARGS) -melf_$(ARCH) arch/$(ARCH)/start.o $(OBJS) \
		kernel.o -o iso/$(ARCH)/boot/kernel.elf

x86: x86_build
x86: ARCH = $(X86)
x86: ELF_F = 32
x86: kernel.elf

x86_64: x86_64_build
x86_64: ARCH = $(X86_64)
x86_64: ELF_F = 64
x86_64: kernel.elf

mkiso_i386: x86
	mkisofs -R \
		-b boot/grub/stage2_eltorito    \
		-no-emul-boot                   \
		-boot-load-size 4               \
		-A $(OS_NAME)					\
		-input-charset utf8             \
		-boot-info-table                \
		-o $(OS_NAME).iso           	\
		iso/i386

mkiso_x86_64: x86_64
	grub-mkrescue -o grub.iso iso/x86_64

run: mkiso_$(ARCH)
	qemu-system-$(ARCH) -enable-kvm -cdrom $(OS_NAME).iso \
			-boot menu=on -drive file=Image.img -m 1G

clean:
	$(RM) *.o
	$(RM) *.elf
	$(RM) *.map
	$(RM) $(OS_NAME).iso
	$(RM) *.out
	$(RM) -r build


