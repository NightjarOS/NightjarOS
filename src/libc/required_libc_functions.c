#include "required_libc_functions.h"

void* memcpy(void *const restrict dest, const void *const restrict src, const size_t count) {
    byte *const restrict dest_ptr = dest;
    const byte *const restrict src_ptr = src;

    for(size_t i = 0; i < count; ++i) {
        dest_ptr[i] = src_ptr[i];
    }

    return dest;
}

void* memset(void *const dest, const int ch, const size_t count) {
    byte *const dest_ptr = dest;

    for(size_t i = 0; i < count; ++i) {
        dest_ptr[i] = (byte) ch;
    }

    return dest;
}

void* memmove(void *const dest, const void *const src, const size_t count) {
    byte *const dest_ptr = dest;
    const byte *const src_ptr = src;

    if(src > dest) {
        for(size_t i = 0; i < count; ++i) {
            dest_ptr[i] = src_ptr[i];
        }
    }
    else if(dest > src) {
        for(size_t i = count; i > 0; --i) {
            dest_ptr[i-1] = src_ptr[i-1];
        }
    }

    return dest;
}

int memcmp(const void *const lhs, const void *const rhs, const size_t count) {
    const byte *const lhs_ptr = lhs;
    const byte *const rhs_ptr = rhs;

    for(size_t i = 0; i < count; ++i) {
        if(lhs_ptr[i] != rhs_ptr[i]) {
            return lhs_ptr[i] < rhs_ptr[i] ? -1 : 1;
        }
    }

    return 0;
}


size_t strlen(const char *const str) {
    size_t len = 0u;
    for(; str[len] != '\0'; ++len);
    return len;
}

int strncmp(const char* lhs, const char* rhs, size_t count) {
    while(count-- != 0) {
        unsigned char lhs_char = (unsigned char) *lhs++;
        unsigned char rhs_char = (unsigned char) *rhs++;
        if(lhs_char != rhs_char) {
            return lhs_char - rhs_char; // even though `lhs_char` and `rhs_char` are unsigned, this can be negative because of integer promotion.
        }
        if(lhs_char == '\0') return 0; // both strings have ended before `count`
    }
    return 0;
}
