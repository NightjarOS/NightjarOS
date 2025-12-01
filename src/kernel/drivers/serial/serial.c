#include "serial.h"

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




// TODO: Remove this later
char* print_digits(uint64_t input, char *const string_ret) {
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

char* print_hex(uint64_t input, char *const string_ret) {
    uint32_t index = 0u;

    do {
        const uint64_t current_input = input % 16u;
        input /= 16u;

        char current_char;
        if(current_input < 10u) {
            current_char = (char) (current_input + 48u);
        } else {
            current_char = (char) (current_input + 55u);
        }

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
