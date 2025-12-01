#include "map_mem.h"

void unconditional_map_page_in_first_2mib(const uint64_t physical_addr, const uint64_t virtual_addr) {
    const uint64_t page_aligned_physical_addr = round_down_to_page(physical_addr);

    // NOTE: `& 0x1FF` grabs the lowest 9 bits since 12 + 4*9 = 48
    // 2^12 = 4096, the page size
    // 2^9 = 512, the number of entries in each table level
    const uint64_t pt_i   = (virtual_addr >> 12) & 0x1FF;
    const uint64_t pdt_i  = (virtual_addr >> 21) & 0x1FF;
    const uint64_t pdpt_i = (virtual_addr >> 30) & 0x1FF;
    const uint64_t pml4_i = (virtual_addr >> 39) & 0x1FF;

    uint64_t *const pml4_virt_addr = (uint64_t*)(KERNEL_VIRT_OFFSET + PM4LT_PHYS_ADDR);
    uint64_t *const pdpt_virt_addr = (uint64_t*)(KERNEL_VIRT_OFFSET + (pml4_virt_addr[pml4_i] & PHYSICAL_ADDRESS_MASK));
    uint64_t *const pdt_virt_addr = (uint64_t*)(KERNEL_VIRT_OFFSET + (pdpt_virt_addr[pdpt_i] & PHYSICAL_ADDRESS_MASK));
    uint64_t *const pt_virt_addr = (uint64_t*)(KERNEL_VIRT_OFFSET + (pdt_virt_addr[pdt_i] & PHYSICAL_ADDRESS_MASK));

    pt_virt_addr[pt_i] = page_aligned_physical_addr | PT_PRESENT | PT_WRITEABLE;

    flush_page_tlb_entry(virtual_addr);
}
