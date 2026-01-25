#pragma once

#include <stddef.h>
#include <stdint.h>

typedef unsigned char byte;

void* memcpy(void *restrict dest, const void *restrict src, size_t count);
void* memset(void* dest, int ch, size_t count);
void* memmove(void* dest, const void* src, size_t count);
int memcmp(const void* lhs, const void* rhs, size_t count);

size_t strlen(const char* str);
int strncmp(const char* lhs, const char* rhs, size_t count);

static inline uint64_t max(const uint64_t lhs, const uint64_t rhs) {
    return (lhs > rhs) ? lhs : rhs;
}
