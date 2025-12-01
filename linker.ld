ENTRY(_start)

KERNEL_PHYS_START = 0x00100000; /* skip first 1 MiB */
KERNEL_VIRT_OFFSET = 0xFFFFFFFF80000000; /* beginning of highest 2GiB */

SECTIONS
{
    . = KERNEL_PHYS_START;

    .boot_trampoline : { /* ENTRY uses the linked address, so we need a low address section for GRUB to jump to as the entry point */
        *(.boot_trampoline)
    }

    . += KERNEL_VIRT_OFFSET;

    . = ALIGN(4K);
    .text : AT(ADDR(.text) - KERNEL_VIRT_OFFSET)
    {
        KEEP(*(.multiboot))
        *(.text)
    }

    . = ALIGN(4K);
    .rodata : AT(ADDR(.rodata) - KERNEL_VIRT_OFFSET)
    {
        *(.rodata)
    }

    . = ALIGN(4K);
    .data : AT(ADDR(.data) - KERNEL_VIRT_OFFSET)
    {
        *(.data)
    }

    /* NOTE: .bss needs to be the last thing mapped to :data, otherwise lots of */
    /* unnecessary zeros will be written to the binary. */
    . = ALIGN(4K);
    .bss : AT(ADDR(.bss) - KERNEL_VIRT_OFFSET)
    {
        *(.bss)
        *(COMMON)
    }

    /* Discard .note.* and .eh_frame* since they may cause issues on some hosts. */
    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }

    kernel_end = .;
}
