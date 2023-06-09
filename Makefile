include define.mk

ELF_F=$(ARCH)
OBJS = $(shell find ./build -name '*.o')
MODULES = arch net fs net/ethernet drivers drivers/storage lib bin mem kernel usermode proc ipc
MODE=
QEMU_ARGS= -enable-kvm -cdrom $(OS_NAME).iso -boot menu=on -drive file=Image.img -m 1G -net nic,model=rtl8139,netdev=mynet0 -netdev user,id=mynet0 -blockdev driver=file,node-name=f0,filename=./floppy.img -device floppy,drive=f0
#-d int -machine accel=tcg -D ./log.txt

build_modules:
	for md in $(MODULES); do \
		$(MAKE) -C $$md $(MODE) $(ARCH) || exit; \
	done

kernel.elf:
	ld $(LD_ARGS) -melf_$(ARCH) $(OBJS) -o iso/$(ARCH)/boot/kernel.elf

# release: clean
release: MODE=release

debug: MODE=debug
debug: QEMU_ARGS=-s -S

x86: ARCH = $(X86)
x86: build_modules
x86: ELF_F = 32
x86: kernel.elf

i386: x86

x86_64: ARCH = $(X86_64)
x86_64: build_modules
x86_64: ELF_F = 64
x86_64: kernel.elf

initrd:
	rm -rf iso/$(ARCH)/boot/ramdisk
	mkdir -p iso/$(ARCH)/boot/ramdisk
	mv build/usermode/* iso/$(ARCH)/boot/ramdisk
	cd iso/$(ARCH)/boot/ramdisk && \
	tar --format=posix -cf ../initrd *

mkiso_i386: kernel.elf initrd
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
	grub-mkrescue -o $(OSNAME).iso iso/i386

debug_run: debug mkiso_$(ARCH)
	qemu-system-$(ARCH) -s -S $(QEMU_ARGS) 

run: mkiso_$(ARCH)
	qemu-system-$(ARCH) $(QEMU_ARGS) 

floppy:
	umount /dev/loop0 ; \
	losetup -d /dev/loop0 ; \
	rm -f floppy.img && \
	dd if=/dev/zero of=floppy.img bs=1024KiB count=10 && \
	losetup /dev/loop0 floppy.img && \
	mkfs -t ext4 /dev/loop0

bochs: mkiso_i386
	bochs -q -f bochsrc.txt

clean:
	$(RM) *.o
	$(RM) *.elf
	$(RM) *.map
	$(RM) $(OS_NAME).iso
	$(RM) *.out
	$(RM) -r build
	$(RM) -r iso/i386/boot/ramdisk
	$(RM) -r iso/i386/boot/kernel.elf
	$(RM) -r iso/x86_64/boot/ramdisk
	$(RM) -r iso/x86_64/boot/kernel.elf


