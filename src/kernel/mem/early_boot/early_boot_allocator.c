#include "early_boot_allocator.h"

static uint64_t current_start_addr = 0u;

void early_boot_alloc_init(const uint64_t last_used_phys_addr) {
    current_start_addr = round_up_to_page(last_used_phys_addr);
}

uint64_t early_boot_alloc(const struct multiboot_tag_mmap *const mmap, const uint64_t requested_size) {
    const uint64_t page_aligned_requested_size = round_up_to_page(requested_size);

    for(uint32_t i = 0; i < (mmap->size - sizeof(struct multiboot_tag_mmap))/mmap->entry_size; ++i) {
        const struct multiboot_mmap_entry current_entry = mmap->entries[i];

        if(current_entry.type != MULTIBOOT_MEMORY_AVAILABLE) {
            continue;
        }

        const uint64_t entry_end = current_entry.addr + current_entry.len;

        uint64_t start = current_entry.addr;
        if(start < current_start_addr) {
            start = current_start_addr;
        }

        start = round_up_to_page(start);

        if(start + page_aligned_requested_size > entry_end) {
            continue;
        }

        current_start_addr = start + page_aligned_requested_size;
        return start;
    }

    halt_and_die("There is not enough memory for early processes!");
    return 0u; // comment just to silence gcc warnings
}
