#pragma once

#include <stdint.h>

#define KERNEL_VIRT_OFFSET 0xFFFFFFFF80000000ULL // beginning of highest 2GiB
#define DIRECT_MAP_OFFSET  0xFFFF800000000000ULL // this is the halfway point of the 4-level virtual address space. This is 2^48 with 1's sign extended into the preceding 16 "fake" bits

#define PT_ADDR_MASK 0xFFFFFFFFFF000ULL

#define KERNEL_V2P(x) ((x) - KERNEL_VIRT_OFFSET)
#define KERNEL_P2V(x) ((x) + KERNEL_VIRT_OFFSET)

#define GENERAL_MEM_V2P(x) ((x) - DIRECT_MAP_OFFSET)
#define GENERAL_MEM_P2V(x) ((x) + DIRECT_MAP_OFFSET)

#define PT_PRESENT 1
#define PT_WRITEABLE 2

#define PHYSICAL_ADDRESS_MASK (0xFFFFFFFFFFULL << 12)

#define NORMAL_PAGE_SIZE 4096ULL
#define HUGE_PAGE_2MIB (1ULL << 21)
#define HUGE_PAGE_1GIB (1ULL << 30)

// bit layout for PDPTE that maps a 1GiB page:
// bit 0 = present bit
// bit 1 = r/w bit
// bit 2 = user bit
// bit 3 = page level write-through bit
// bit 4 = page level cache disable bit
// bit 5 = accessed bit
// bit 6 = dirty bit
// bit 7 = page size (must be 1 to enable huge 1GiB page entry)
// bit 8 = global page bit
// bits 9 and 10 = ignored
// bit 11 = ignored for normal paging, only used for HLAT paging (which we don't use)
// bit 12 = PAT bit
// bits [13, 29] = reserved (must be 0)
// bits [30, 51] = physical address of 1GiB aligned page
// bits [52, 58] = ignored
// bits [59, 62] = protection key
// bit 63 = execute-disable bit
#define PDPTE_PRESENT 1
#define PDPTE_WRITEABLE 2
#define PDPTE_HUGE_PAGE (1ULL << 7)
#define PDPTE_GLOBAL_PAGE (1ULL << 8)
#define PDPTE_DISABLE_EXECUTE (1ULL << 63)

extern char kernel_end; // &kernel_end = kernel end addr

extern char pml4t; // &pml4t = higher half kernel virtual address mapping of the pml4t
#define PM4LT_PHYS_ADDR KERNEL_V2P((uint64_t)&pml4t)

extern uint32_t multiboot_total_size; // This is set in the boot stub since it is easier than mapping a page just to read the multiboot structure size.

extern uint64_t early_single_page_virt_page_addr; // This is the virtual page right after `&kernel_end` and is used in order to traverse through the multiboot structure and read the memory map

static inline uint64_t round_up(const uint64_t unrounded_val, const uint64_t desired_factor) {
    return (unrounded_val + desired_factor - 1ULL) & ~(desired_factor - 1ULL);
}

static inline uint64_t round_up_to_page(const uint64_t addr) {
    return round_up(addr, NORMAL_PAGE_SIZE);
}

static inline uint64_t round_down(const uint64_t unrounded_val, const uint64_t desired_factor) {
    return unrounded_val & ~(desired_factor - 1ULL);
}

static inline uint64_t round_down_to_page(const uint64_t addr) {
    return round_down(addr, NORMAL_PAGE_SIZE);
}

static inline uint64_t offset(const uint64_t val, const uint64_t factor) {
    return val & (factor - 1ULL);
}

static inline uint64_t offset_in_page(const uint64_t addr) {
    return offset(addr, NORMAL_PAGE_SIZE);
}
