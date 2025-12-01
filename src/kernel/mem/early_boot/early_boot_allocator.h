#pragma once

#include <stdint.h>

#include <kernel/boot/multiboot.h>

#include <kernel/mem/mem_constants.h>
#include <kernel/error/error.h>

void early_boot_alloc_init(uint64_t last_used_phys_addr);
uint64_t early_boot_alloc(const struct multiboot_tag_mmap* mmap, uint64_t requested_size);
