#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <types.h>
#include <libc/required_libc_functions.h>

#include "multiboot.h"


uint8_t inb(const uint16_t port) {
    unsigned char v;

    asm volatile ("inb %w1,%0":"=a" (v):"Nd" (port));
    return v;
}

void outb(const uint16_t port, const uint8_t value) {
    asm volatile ("outb %b0,%w1": :"a" (value), "Nd" (port));
}


#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

bool serial_init(void) {
   outb(COM1 + 1, 0x00);    // Disable all interrupts
   outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(COM1 + 1, 0x00);    //                  (hi byte)
   outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   outb(COM1 + 4, 0x1E);    // Set in loopback mode, test the serial chip
   outb(COM1 + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    if(inb(COM1 + 0) != 0xAE) return false;

    outb(COM1 + 4, 0x0F);
    return true;
}

uint8_t serial_received(void) {
    return inb(COM1 + 5) & 1u;
}

uint8_t serial_read(void) {
    while (serial_received() == 0);

    return inb(COM1);
}

uint8_t is_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20u;
}

void serial_putchar(const char c) {
    while (is_transmit_empty() == 0);

    outb(COM1, (uint8_t)c);
}

void serial_write(const char *const text, const size_t size) {
    for(size_t i = 0u; i < size; ++i) {
        serial_putchar(text[i]);
    }
}

void serial_writestring(const char *const text) {
    serial_write(text, strlen(text));
}

static void halt(void) {
    for(;;) {
        asm("hlt");
    }
}

char* kint_to_str(uint64_t input, char *const string_ret) {
    uint32_t index = 0u;

    do {
        const uint64_t current_input = input % 10u;
        input /= 10u;

        const char current_char = (char) (current_input + 48u);

        string_ret[index++] = current_char;
    } while(input);

    string_ret[index] = '\0';

    for(size_t i = 0u; i < index/2u; ++i) {
        const char temp = string_ret[i];
        string_ret[i] = string_ret[index-i-1u];
        string_ret[index-i-1u] = temp;
    }

    return string_ret;
}

void kernel_main(const uint64_t mboot_magic, const uint64_t mboot_header) {
    if(serial_init()) {
        serial_writestring("Serial driver works.\n");
    } else {
        halt();
    }

    if (mboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        char expected_str[256];
        char actual_str[256];
        serial_writestring("Invalid Multiboot Magic!\n");
        serial_writestring("Expected: ");
        serial_writestring(kint_to_str(MULTIBOOT2_BOOTLOADER_MAGIC, expected_str));
        serial_writestring(", Actual: ");
        serial_writestring(kint_to_str(mboot_magic, actual_str));
        serial_writestring(".\n");
    } else {
        serial_writestring("The multiboot structure was loaded properly.\n");
    }

    halt();
}
