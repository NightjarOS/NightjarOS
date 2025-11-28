ENTRY(_start)

KERNEL_START = 0x00100000;

SECTIONS
{
    . = KERNEL_START;

    .text : AT(ADDR(.text))
    {
        KEEP(*(.multiboot))
        *(.text)
    }
    . = ALIGN(4K);
    .rodata : AT(ADDR(.rodata))
    {
        *(.rodata)
    }

    . = ALIGN(4K);
    .data : AT(ADDR(.data))
    {
        *(.data)
    }
    . = ALIGN(4K);
    .bss : AT(ADDR(.bss))
    {
        *(.COMMON)
        *(.bss)
    }
}
