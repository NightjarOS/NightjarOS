#pragma once

#include <stdint.h>

#include "mem_constants.h"

// NOTE: Only used for memory mapping prior to getting the memory and/or setting up linear shifted RAM address space
// NOTE: Make sure you know what you are doing since this will blindly index in and change the pt entry
void unconditional_map_page_in_first_2mib(uint64_t physical_addr, uint64_t virtual_addr);

static inline void flush_page_tlb_entry(const uint64_t virt_addr) {
    asm volatile("invlpg (%0)" ::"r" (virt_addr) : "memory");
}

static inline void reload_cr3(const uint64_t pml4_phys_addr) {
    asm volatile("movq %0, %%cr3" :: "r" (pml4_phys_addr) : "memory");
}

static inline void early_single_page_virt_page_init(void) {
    early_single_page_virt_page_addr = round_up_to_page((uint64_t)&kernel_end);
}
