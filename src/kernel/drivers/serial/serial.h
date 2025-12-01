#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <kernel/io/port_io.h>
#include <libc/required_libc_functions.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

//returns whether or not the serial is faulty. true = serial works properly. false = serial is faulty.
bool serial_init(void);

static inline uint8_t serial_received(void) {
    return inb(COM1 + 5) & 1u;
}

static inline uint8_t serial_read(void) {
    while (serial_received() == 0);

    return inb(COM1);
}

static inline uint8_t is_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20u;
}

static inline void serial_putchar(const char c) {
    while (is_transmit_empty() == 0);

    outb(COM1, (uint8_t)c);
}

static inline void serial_write(const char *const text, const size_t size) {
    for(size_t i = 0u; i < size; ++i) {
        serial_putchar(text[i]);
    }
}

static inline void serial_writestring(const char *const text) {
    serial_write(text, strlen(text));
}




// TODO: Remove this later
char* print_digits(uint64_t input, char* string_ret);
char* print_hex(uint64_t input, char* string_ret);
