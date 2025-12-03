override OUTPUT := nightjaros

TOOLCHAIN := x86_64-elf-

CC := $(TOOLCHAIN_PREFIX)gcc
LD := $(TOOLCHAIN_PREFIX)ld

# Internal C flags that should not be changed by the user.
override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu17 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections \
    -m64 \
    -march=x86-64 \
    -mabi=sysv \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mno-red-zone \
    -mcmodel=kernel

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
    -I src \
    $(CPPFLAGS) \
    -MMD \
    -MP

# Internal nasm flags that should not be changed by the user.
override NASMFLAGS := \
    -f elf64 \
    $(patsubst -g,-g -F dwarf,$(NASMFLAGS)) \
    -Wall

override LDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    -T linker.ld

override SRCFILES := $(shell find -L src -type f 2>/dev/null | LC_ALL=C sort)
override CFILES := $(filter %.c,$(SRCFILES))
override ASFILES := $(filter %.asm,$(SRCFILES))
override OBJ := $(addprefix obj/,$(CFILES:.c=.c.o) $(ASFILES:.asm=.asm.o))
override HEADER_DEPS := $(addprefix obj/,$(CFILES:.c=.c.d) $(ASFILES:.asm=.asm.d))

.PHONY: all build_iso run clean
.SUFFIXES: .o .c .asm

all : build_iso

-include $(HEADER_DEPS)

build_iso : bin/$(OUTPUT)
	mkdir -p isodir/
	mkdir -p isodir/boot/
	mkdir -p isodir/boot/grub/
	cp bin/$(OUTPUT) isodir/boot/$(OUTPUT)
	cp ramdisk.img isodir/boot/ramdisk.img
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub2-mkrescue -o $(OUTPUT).iso isodir

run : build_iso
	qemu-system-x86_64 \
	-machine q35 \
	-m 16G \
	-drive if=pflash,format=raw,readonly=on,file=./ovmf/OVMF_CODE.fd \
	-drive if=pflash,format=raw,file=./ovmf/OVMF_VARS.fd \
	-cdrom $(OUTPUT).iso \
	-drive file=ramdisk.img,format=raw \
	-serial stdio

bin/$(OUTPUT): linker.ld $(OBJ)
	mkdir -p "$(dir $@)"
	$(LD) $(LDFLAGS) $(OBJ) -o $@

obj/%.c.o: %.c
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj/%.asm.o: %.asm
	mkdir -p "$(dir $@)"
	nasm $(NASMFLAGS) $< -o $@

clean:
	rm -rf bin obj isodir $(OUTPUT).iso
