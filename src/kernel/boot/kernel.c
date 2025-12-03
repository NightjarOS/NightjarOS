#include <stdint.h>
#include <stddef.h>

#include <types.h>
#include <kernel/drivers/serial/serial.h>
#include <kernel/error/error.h>

#include <kernel/mem/mem_constants.h>
#include <kernel/mem/map_mem.h>
#include <kernel/mem/early_boot/early_boot_allocator.h>
#include <kernel/mem/phys/phys_mem_allocator.h>

#include "multiboot.h"

void kernel_main(const uint64_t mboot_magic, const uint64_t mboot_header_phys_addr) {
    if(serial_init()) {
        serial_writestring("Serial driver works.\n");
    } else {
        halt();
    }

    char str_buf[128];
    if (mboot_magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        serial_writestring("The multiboot structure was loaded properly.\n");
    } else {
        halt_and_die("Bad multiboot magic detected.");
    }

    early_single_page_virt_page_init();

    // TODO: This works, but is atrocious to read/look at. We should refactor it immensely.
    const struct multiboot_tag_mmap* mmap = NULL;
    uint64_t mmap_phys_addr = 0u;
    uint64_t current_phys_ptr = mboot_header_phys_addr + sizeof(uint64_t);
    const struct multiboot_tag* current_virt_ptr = (struct multiboot_tag*) (early_single_page_virt_page_addr + offset_in_page(current_phys_ptr));
    unconditional_map_page_in_first_2mib(current_phys_ptr, (uint64_t)current_virt_ptr);
    for(; current_virt_ptr->type != MULTIBOOT_TAG_TYPE_END; current_phys_ptr = (uint64_t)(current_phys_ptr  + round_up(current_virt_ptr->size, sizeof(uint64_t)))) {
        current_virt_ptr = (struct multiboot_tag*)(early_single_page_virt_page_addr + offset_in_page(current_phys_ptr));
        unconditional_map_page_in_first_2mib(current_phys_ptr, (uint64_t)current_virt_ptr);
        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_MMAP) {
            mmap_phys_addr = current_phys_ptr;
            mmap = (struct multiboot_tag_mmap*)current_virt_ptr;
            // NOTE: The memory map is not guaranteed to be a single page, so we should replace the below check with the same traversal logic we used above when reading it later
            const uint64_t mmap_base = (uint64_t)mmap;
            const uint64_t mmap_end = mmap_base + mmap->size - 1u;
            if(round_down_to_page(mmap_base) != round_down_to_page(mmap_end)) {
                halt_and_die("mmap crosses multiple pages.");
            }
            break;
        }
    }
    uint64_t total_available_memory = 0u;
    uint64_t amount_to_map = 0u;
    if(mmap == NULL) {
        serial_writestring("mmap could not be found.\n");
    } else {
        serial_writestring("mmap detected.\n");
        for(uint32_t i = 0; i < (mmap->size - sizeof(struct multiboot_tag_mmap))/mmap->entry_size; ++i) {
            const struct multiboot_mmap_entry current_entry = mmap->entries[i];
            serial_writestring("mmap_entry : { [");
            serial_writestring(print_hex(current_entry.addr, str_buf)); // TODO: Print hex
            serial_writestring("--");
            serial_writestring(print_hex(current_entry.addr+current_entry.len-1u, str_buf));
            serial_writestring("], type: ");
            if(current_entry.type == MULTIBOOT_MEMORY_AVAILABLE) {
                serial_writestring("AVAILABLE.");
                total_available_memory += current_entry.len;
            }
            else if(current_entry.type == MULTIBOOT_MEMORY_RESERVED) {
                serial_writestring("RESERVED.");
            }
            else if(current_entry.type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
                serial_writestring("ACPI_RECLAIMABLE.");
            }
            else if(current_entry.type == MULTIBOOT_MEMORY_NVS) {
                serial_writestring("NVS.");
            }
            else if(current_entry.type == MULTIBOOT_MEMORY_BADRAM) {
                serial_writestring("BADRAM.");
            }
            else {
                serial_writestring("UNKNOWN RESERVED: ");
                serial_writestring(print_digits(current_entry.type, str_buf));
                serial_writestring(".");
            }
            if(current_entry.type != MULTIBOOT_MEMORY_RESERVED) {
                // NOTE: GRUB includes a dummy RESERVED entry at the end of the memory map that contains all the non-existent physical memory addresses (i.e. the other TiB's of RAM that don't exist if on a 16GiB machine).
                //  To avoid allocating a ton of extra memory for page tables to memory that does not even exist, we just skip the RESERVED entries to calculate the effective "max physical memory address" for the linear map.
                amount_to_map = max(amount_to_map, current_entry.addr + current_entry.len);
            }
            serial_writestring("}\n");
        }
        serial_writestring("Total available RAM in bytes (in hex): ");
        serial_writestring(print_hex(total_available_memory, str_buf));
        serial_writestring(".\n");
    }

    early_boot_alloc_init(max(KERNEL_V2P((uint64_t)&kernel_end), mboot_header_phys_addr+multiboot_total_size));

    const uint64_t number_of_huge_pages = round_up(amount_to_map, HUGE_PAGE_1GIB)/HUGE_PAGE_1GIB;
    const uint64_t number_of_pdpte_pages = round_up(number_of_huge_pages, 512)/512ULL;
    if(number_of_pdpte_pages != 1) {
        halt_and_die("More than 512GiB of memory detected.");
    }

    const uint64_t pdpte_phys_page_addr = early_boot_alloc(mmap, number_of_pdpte_pages*NORMAL_PAGE_SIZE);
    uint64_t *const pdpte_virtual_page_ptr = (uint64_t*)early_single_page_virt_page_addr;
    unconditional_map_page_in_first_2mib(pdpte_phys_page_addr, early_single_page_virt_page_addr);
    memset((void*)early_single_page_virt_page_addr, 0, NORMAL_PAGE_SIZE);
    for(uint64_t i = 0; i < number_of_huge_pages; ++i) {
        pdpte_virtual_page_ptr[i] = (HUGE_PAGE_1GIB * i) | PDPTE_PRESENT | PDPTE_WRITEABLE | PDPTE_HUGE_PAGE | PDPTE_GLOBAL_PAGE | PDPTE_DISABLE_EXECUTE;
    }
    uint64_t *const pml4_virt_addr = (uint64_t*)(KERNEL_VIRT_OFFSET + PM4LT_PHYS_ADDR);
    pml4_virt_addr[256] = (pdpte_phys_page_addr & PT_ADDR_MASK) | PT_PRESENT | PT_WRITEABLE;

    reload_cr3(PM4LT_PHYS_ADDR);

    const uint64_t total_number_of_pages = round_down_to_page(amount_to_map)/NORMAL_PAGE_SIZE;
    const uint64_t total_number_of_pages_rounded_up = round_up(total_number_of_pages, 64ULL);
    const uint64_t total_number_of_uint64t_entries = total_number_of_pages_rounded_up/64ULL;
    const uint64_t total_number_of_excess_pages = total_number_of_pages_rounded_up - total_number_of_pages;
    const uint64_t phys_mem_physical_memory = early_boot_alloc((struct multiboot_tag_mmap*) GENERAL_MEM_P2V(mmap_phys_addr), total_number_of_uint64t_entries*sizeof(uint64_t));

    phys_mem_alloc_init((uint64_t*)GENERAL_MEM_P2V(phys_mem_physical_memory), total_number_of_uint64t_entries);

    phys_mem_reserve_pages(0x00100000ULL, KERNEL_V2P((uint64_t)&kernel_end) - 0x00100000ULL);
    phys_mem_reserve_pages(mboot_header_phys_addr, multiboot_total_size);
    phys_mem_reserve_pages(total_number_of_pages*NORMAL_PAGE_SIZE, total_number_of_excess_pages*NORMAL_PAGE_SIZE);

    serial_writestring("Testing linear map:\n");
    const struct multiboot_tag_mmap *const memory_map_virtual_ptr = (struct multiboot_tag_mmap*) GENERAL_MEM_P2V(mmap_phys_addr);
    for(uint32_t i = 0; i < (memory_map_virtual_ptr->size - sizeof(struct multiboot_tag_mmap))/memory_map_virtual_ptr->entry_size; ++i) {
        const struct multiboot_mmap_entry current_entry = memory_map_virtual_ptr->entries[i];
        serial_writestring("mmap_entry : { [");
        serial_writestring(print_hex(current_entry.addr, str_buf)); // TODO: Print hex
        serial_writestring("--");
        serial_writestring(print_hex(current_entry.addr+current_entry.len-1u, str_buf));
        serial_writestring("], type: ");
        if(current_entry.type == MULTIBOOT_MEMORY_AVAILABLE) {
            serial_writestring("AVAILABLE.");
        }
        else if(current_entry.type == MULTIBOOT_MEMORY_RESERVED) {
            serial_writestring("RESERVED.");

            const uint64_t start = current_entry.addr;
            uint64_t end   = current_entry.addr + current_entry.len; // exclusive
            const uint64_t tracked_end = total_number_of_pages * NORMAL_PAGE_SIZE;
            if (start < tracked_end) {
                if (end > tracked_end) {
                    end = tracked_end; // clip to bitmap end
                }
                phys_mem_reserve_pages(start, end - start);
            }
        }
        else if(current_entry.type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
            serial_writestring("ACPI_RECLAIMABLE.");
            phys_mem_reserve_pages(current_entry.addr, current_entry.len);
        }
        else if(current_entry.type == MULTIBOOT_MEMORY_NVS) {
            serial_writestring("NVS.");
            phys_mem_reserve_pages(current_entry.addr, current_entry.len);
        }
        else if(current_entry.type == MULTIBOOT_MEMORY_BADRAM) {
            serial_writestring("BADRAM.");
            phys_mem_reserve_pages(current_entry.addr, current_entry.len);
        }
        else {
            serial_writestring("UNKNOWN RESERVED: ");
            serial_writestring(print_digits(current_entry.type, str_buf));
            serial_writestring(".");
            phys_mem_reserve_pages(current_entry.addr, current_entry.len);
        }
        serial_writestring("}\n");
    }



    halt();
}
