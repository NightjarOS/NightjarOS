#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <types.h>


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

static void halt(void) {
    for(;;) {
        serial_write("hello, world", 12);
        asm("hlt");
    }
}

void kernel_main(void) {
    halt();
}
