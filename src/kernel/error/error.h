#pragma once

#include <kernel/drivers/serial/serial.h>

static inline void halt(void) {
    for(;;) {
        asm("hlt");
    }
}

static inline void halt_and_die(const char *const str) {
    serial_writestring(str);
    halt();
}
