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
