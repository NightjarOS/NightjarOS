#pragma once

#include <stdint.h>


static inline uint8_t inb(const uint16_t port) {
    unsigned char v;

    asm volatile ("inb %w1,%0":"=a" (v):"Nd" (port));
    return v;
}

static inline void outb(const uint16_t port, const uint8_t value) {
    asm volatile ("outb %b0,%w1": :"a" (value), "Nd" (port));
}
