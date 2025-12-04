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

struct ramdisk_metadata {
    uint64_t mod_start;
    uint64_t mod_end;
};

// this will also setup the temporary virtual memory mapping for the memory map at `(early_single_page_virt_page_addr + offset_in_page(mmap_physical_addr))`
uint64_t get_memory_map(const uint64_t mboot_header_phys_addr) {
    uint64_t current_phys_ptr = mboot_header_phys_addr + 2 * sizeof(multiboot_uint32_t);

    for(;;) {
        const struct multiboot_tag *const current_virt_ptr = (struct multiboot_tag*) (early_single_page_virt_page_addr + offset_in_page(current_phys_ptr));
        unconditional_map_page_in_first_2mib(current_phys_ptr, (uint64_t)current_virt_ptr);

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_END) break;

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_MMAP) {
            // NOTE: The memory map is not guaranteed to be a single page, so we should replace the below check with the same traversal logic we used above when reading it later
            const uint64_t mmap_base = (uint64_t)current_virt_ptr;
            const uint64_t mmap_end = mmap_base + current_virt_ptr->size - 1u;

            if(round_down_to_page(mmap_base) != round_down_to_page(mmap_end)) {
                halt_and_die("mmap crosses multiple pages.");
            }

            return current_phys_ptr;
        }

        current_phys_ptr = (uint64_t)(current_phys_ptr  + round_up(current_virt_ptr->size, MULTIBOOT_TAG_ALIGN));
    }

    halt_and_die("Memory map not found.");

    return 0u; // return just to silence gcc warnings
}

struct memory_size_info {
    uint64_t total_available_memory;
    uint64_t amount_to_map;
};

struct memory_size_info get_memory_size_info(const struct multiboot_tag_mmap *const memory_map_virtual_ptr) {
    uint64_t total_available_memory = 0u;
    uint64_t amount_to_map = 0u;
    for(uint32_t i = 0; i < (memory_map_virtual_ptr->size - sizeof(struct multiboot_tag_mmap))/memory_map_virtual_ptr->entry_size; ++i) {
        const struct multiboot_mmap_entry current_entry = memory_map_virtual_ptr->entries[i];
        if(current_entry.type == MULTIBOOT_MEMORY_AVAILABLE) {
            total_available_memory += current_entry.len;
        }

        if(current_entry.type != MULTIBOOT_MEMORY_RESERVED) {
            // NOTE: GRUB includes a dummy RESERVED entry at the end of the memory map that contains all the non-existent physical memory addresses (i.e. the other TiB's of RAM that don't exist if on a 16GiB machine).
            //  To avoid allocating a ton of extra memory for page tables to memory that does not even exist, we just skip the RESERVED entries to calculate the effective "max physical memory address" for the linear map.
            amount_to_map = max(amount_to_map, current_entry.addr + current_entry.len);
        }
    }

    return (struct memory_size_info) { total_available_memory, amount_to_map };
}

void print_memory_map(const struct multiboot_tag_mmap *const memory_map_virtual_ptr, const struct memory_size_info mem_size_info) {
    char str_buf[128];
    serial_writestring("Memory map:\n");
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
        serial_writestring("}\n");
    }

    serial_writestring("Total available RAM in bytes (in hex): ");
    serial_writestring(print_hex(mem_size_info.total_available_memory, str_buf));
    serial_writestring(".\n");
}

uint64_t setup_linear_mapping(const struct multiboot_tag_mmap *const memory_map_virtual_ptr, const struct memory_size_info mem_size_info) {
    const uint64_t number_of_huge_pages = round_up(mem_size_info.amount_to_map, HUGE_PAGE_1GIB)/HUGE_PAGE_1GIB;
    const uint64_t number_of_pdpte_pages = round_up(number_of_huge_pages, 512)/512ULL;
    if(number_of_pdpte_pages != 1) {
        halt_and_die("More than 512GiB of memory detected.");
    }

    const uint64_t pdpte_phys_page_addr = early_boot_alloc(memory_map_virtual_ptr, number_of_pdpte_pages*NORMAL_PAGE_SIZE);
    uint64_t *const pdpte_virtual_page_ptr = (uint64_t*)early_single_page_virt_page_addr;
    unconditional_map_page_in_first_2mib(pdpte_phys_page_addr, early_single_page_virt_page_addr);
    memset((void*)early_single_page_virt_page_addr, 0, NORMAL_PAGE_SIZE);
    for(uint64_t i = 0; i < number_of_huge_pages; ++i) {
        pdpte_virtual_page_ptr[i] = (HUGE_PAGE_1GIB * i) | PDPTE_PRESENT | PDPTE_WRITEABLE | PDPTE_HUGE_PAGE | PDPTE_GLOBAL_PAGE | PDPTE_DISABLE_EXECUTE;
    }
    uint64_t *const pml4_virt_addr = (uint64_t*)(KERNEL_VIRT_OFFSET + PM4LT_PHYS_ADDR);
    pml4_virt_addr[256] = (pdpte_phys_page_addr & PT_ADDR_MASK) | PT_PRESENT | PT_WRITEABLE;

    reload_cr3(PM4LT_PHYS_ADDR);
    return pdpte_phys_page_addr;
}

void reserve_unavailable_physical_memory(const uint64_t mboot_header_phys_addr, const uint64_t mmap_physical_addr, const struct memory_size_info mem_size_info, const struct ramdisk_metadata ramdisk_metadata, const uint64_t pdpte_phys_page_addr) {
    const uint64_t total_number_of_pages = round_down_to_page(mem_size_info.amount_to_map)/NORMAL_PAGE_SIZE;
    const uint64_t total_number_of_pages_rounded_up = round_up(total_number_of_pages, 64ULL);
    const uint64_t total_number_of_uint64t_entries = total_number_of_pages_rounded_up/64ULL;
    const uint64_t total_number_of_excess_pages = total_number_of_pages_rounded_up - total_number_of_pages;
    const uint64_t phys_mem_physical_memory = early_boot_alloc((struct multiboot_tag_mmap*) GENERAL_MEM_P2V(mmap_physical_addr), total_number_of_uint64t_entries*sizeof(uint64_t));

    phys_mem_alloc_init((uint64_t*)GENERAL_MEM_P2V(phys_mem_physical_memory), total_number_of_uint64t_entries);

    phys_mem_reserve_pages(0x00100000ULL, KERNEL_V2P((uint64_t)&kernel_end) - 0x00100000ULL);
    phys_mem_reserve_pages(mboot_header_phys_addr, multiboot_total_size);
    phys_mem_reserve_pages(total_number_of_pages*NORMAL_PAGE_SIZE, total_number_of_excess_pages*NORMAL_PAGE_SIZE);
    phys_mem_reserve_pages(ramdisk_metadata.mod_start, ramdisk_metadata.mod_end - ramdisk_metadata.mod_start);
    phys_mem_reserve_pages(phys_mem_physical_memory, total_number_of_uint64t_entries*sizeof(uint64_t));
    phys_mem_reserve_pages(pdpte_phys_page_addr, NORMAL_PAGE_SIZE);

    const struct multiboot_tag_mmap *const memory_map_virtual_ptr = (struct multiboot_tag_mmap*) GENERAL_MEM_P2V(mmap_physical_addr);
    for(uint32_t i = 0; i < (memory_map_virtual_ptr->size - sizeof(struct multiboot_tag_mmap))/memory_map_virtual_ptr->entry_size; ++i) {
        const struct multiboot_mmap_entry current_entry = memory_map_virtual_ptr->entries[i];
        if(current_entry.type == MULTIBOOT_MEMORY_RESERVED) {
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
        else if(current_entry.type != MULTIBOOT_MEMORY_AVAILABLE) {
            phys_mem_reserve_pages(current_entry.addr, current_entry.len);
        }
    }
}

void dump_multiboot_tags(const uint64_t mboot_header_phys_addr) {
    uint64_t current_phys_ptr = mboot_header_phys_addr + 2 * sizeof(multiboot_uint32_t);

    char str_buf[128];

    serial_writestring("All multiboot tags found: {\n");
    for(;;) {
        const struct multiboot_tag *const current_virt_ptr = (struct multiboot_tag*) GENERAL_MEM_P2V(current_phys_ptr);

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_END) break;

        switch(current_virt_ptr->type) {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
                serial_writestring("MULTIBOOT_TAG_TYPE_CMDLINE\n");
                break;
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
                serial_writestring("MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME\n");
                break;
            case MULTIBOOT_TAG_TYPE_MODULE:
                serial_writestring("MULTIBOOT_TAG_TYPE_MODULE\n");
                break;
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
                serial_writestring("MULTIBOOT_TAG_TYPE_BASIC_MEMINFO\n");
                break;
            case MULTIBOOT_TAG_TYPE_BOOTDEV:
                serial_writestring("MULTIBOOT_TAG_TYPE_BOOTDEV\n");
                break;
            case MULTIBOOT_TAG_TYPE_MMAP:
                serial_writestring("MULTIBOOT_TAG_TYPE_MMAP\n");
                break;
            case MULTIBOOT_TAG_TYPE_VBE:
                serial_writestring("MULTIBOOT_TAG_TYPE_VBE\n");
                break;
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
                serial_writestring("MULTIBOOT_TAG_TYPE_FRAMEBUFFER\n");
                break;
            case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
                serial_writestring("MULTIBOOT_TAG_TYPE_ELF_SECTIONS\n");
                break;
            case MULTIBOOT_TAG_TYPE_APM:
                serial_writestring("MULTIBOOT_TAG_TYPE_APM\n");
                break;
            case MULTIBOOT_TAG_TYPE_EFI32:
                serial_writestring("MULTIBOOT_TAG_TYPE_EFI32\n");
                break;
            case MULTIBOOT_TAG_TYPE_EFI64:
                serial_writestring("MULTIBOOT_TAG_TYPE_EFI64\n");
                break;
            case MULTIBOOT_TAG_TYPE_SMBIOS:
                serial_writestring("MULTIBOOT_TAG_TYPE_SMBIOS\n");
                break;
            case MULTIBOOT_TAG_TYPE_ACPI_OLD:
                serial_writestring("MULTIBOOT_TAG_TYPE_ACPI_OLD\n");
                break;
            case MULTIBOOT_TAG_TYPE_ACPI_NEW:
                serial_writestring("MULTIBOOT_TAG_TYPE_ACPI_NEW\n");
                break;
            case MULTIBOOT_TAG_TYPE_NETWORK:
                serial_writestring("MULTIBOOT_TAG_TYPE_NETWORK\n");
                break;
            case MULTIBOOT_TAG_TYPE_EFI_MMAP:
                serial_writestring("MULTIBOOT_TAG_TYPE_EFI_MMAP\n");
                break;
            case MULTIBOOT_TAG_TYPE_EFI_BS:
                serial_writestring("MULTIBOOT_TAG_TYPE_EFI_BS\n");
                break;
            case MULTIBOOT_TAG_TYPE_EFI32_IH:
                serial_writestring("MULTIBOOT_TAG_TYPE_EFI32_IH\n");
                break;
            case MULTIBOOT_TAG_TYPE_EFI64_IH:
                serial_writestring("MULTIBOOT_TAG_TYPE_EFI64_IH\n");
                break;
            case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
                serial_writestring("MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR\n");
                break;
            default:
                serial_writestring("Unknown type: ");
                serial_writestring(print_digits(current_virt_ptr->type, str_buf));
                serial_writestring("\n");
        }

        current_phys_ptr = (uint64_t)(current_phys_ptr  + round_up(current_virt_ptr->size, MULTIBOOT_TAG_ALIGN));
    }
    serial_writestring("}\n");
}

struct multiboot_tag_framebuffer* get_framebuffer(const uint64_t mboot_header_phys_addr) {
    uint64_t current_phys_ptr = mboot_header_phys_addr + 2 * sizeof(multiboot_uint32_t);

    for(;;) {
        const struct multiboot_tag *const current_virt_ptr = (struct multiboot_tag*) GENERAL_MEM_P2V(current_phys_ptr);

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_END) break;

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            return (struct multiboot_tag_framebuffer*) current_virt_ptr;
        }

        current_phys_ptr = (uint64_t)(current_phys_ptr  + round_up(current_virt_ptr->size, MULTIBOOT_TAG_ALIGN));
    }

    halt_and_die("Framebuffer not found.");

    return NULL; // return just to silence gcc warnings
}

struct ramdisk_metadata get_ramdisk_metadata(const uint64_t mboot_header_phys_addr) {
    uint64_t current_phys_ptr = mboot_header_phys_addr + 2 * sizeof(multiboot_uint32_t);

    for(;;) {
        const struct multiboot_tag *const current_virt_ptr = (struct multiboot_tag*) (early_single_page_virt_page_addr + offset_in_page(current_phys_ptr));
        unconditional_map_page_in_first_2mib(current_phys_ptr, (uint64_t)current_virt_ptr);

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_END) break;

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_MODULE) {
            struct multiboot_tag_module *const module = (struct multiboot_tag_module*) current_virt_ptr;
            return (struct ramdisk_metadata) { module->mod_start, module->mod_end };
        }

        current_phys_ptr = (uint64_t)(current_phys_ptr  + round_up(current_virt_ptr->size, MULTIBOOT_TAG_ALIGN));
    }

    halt_and_die("Ramdisk not found.");

    return (struct ramdisk_metadata) { 0, 0 }; // return just to silence gcc warnings
}

struct multiboot_tag_module* get_ramdisk(const uint64_t mboot_header_phys_addr) {
    uint64_t current_phys_ptr = mboot_header_phys_addr + 2 * sizeof(multiboot_uint32_t);

    for(;;) {
        const struct multiboot_tag *const current_virt_ptr = (struct multiboot_tag*) GENERAL_MEM_P2V(current_phys_ptr);

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_END) break;

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_MODULE) {
            return (struct multiboot_tag_module*) current_virt_ptr;
        }

        current_phys_ptr = (uint64_t)(current_phys_ptr  + round_up(current_virt_ptr->size, MULTIBOOT_TAG_ALIGN));
    }

    halt_and_die("Ramdisk not found.");

    return NULL; // return just to silence gcc warnings
}

void kernel_main(const uint64_t mboot_magic, const uint64_t mboot_header_phys_addr) {
    if(serial_init()) {
        serial_writestring("Serial driver works.\n");
    } else {
        halt();
    }

    if (mboot_magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        serial_writestring("The multiboot structure was loaded properly.\n");
    } else {
        halt_and_die("Bad multiboot magic.");
    }

    early_single_page_virt_page_init();

    struct ramdisk_metadata ramdisk_metadata = get_ramdisk_metadata(mboot_header_phys_addr);

    const uint64_t mmap_physical_addr = get_memory_map(mboot_header_phys_addr);
    // temporary mapping until we set up linear map:
    const struct multiboot_tag_mmap* mmap_virtual_ptr = (struct multiboot_tag_mmap*) (early_single_page_virt_page_addr + offset_in_page(mmap_physical_addr));

    const uint64_t first_unused_memory_address = max(max(KERNEL_V2P((uint64_t)&kernel_end), mboot_header_phys_addr+multiboot_total_size), ramdisk_metadata.mod_end);
    early_boot_alloc_init(first_unused_memory_address);

    struct memory_size_info mem_size_info = get_memory_size_info(mmap_virtual_ptr);

    const uint64_t pdpte_phys_page_addr = setup_linear_mapping(mmap_virtual_ptr, mem_size_info);

    // use the linear map:
    mmap_virtual_ptr = (struct multiboot_tag_mmap*) GENERAL_MEM_P2V(mmap_physical_addr);

    reserve_unavailable_physical_memory(mboot_header_phys_addr, mmap_physical_addr, mem_size_info, ramdisk_metadata, pdpte_phys_page_addr);





    dump_multiboot_tags(mboot_header_phys_addr);

    print_memory_map(mmap_virtual_ptr, mem_size_info);

    const struct multiboot_tag_framebuffer *const fb_tag = get_framebuffer(mboot_header_phys_addr);
    volatile uint32_t *fb_ptr = (uint32_t*) GENERAL_MEM_P2V(fb_tag->common.framebuffer_addr);
    uint32_t fb_pitch  = fb_tag->common.framebuffer_pitch;
    uint8_t  fb_bpp    = fb_tag->common.framebuffer_bpp;
    uint8_t  fb_type   = fb_tag->common.framebuffer_type;
    if (fb_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB || fb_bpp != 32) {
        halt_and_die("Unsupported framebuffer format.");
    }
    for (size_t i = 0; i < 100; i++) {
        fb_ptr[i * (fb_pitch / 4) + i] = 0xffffff;
    }

    const struct multiboot_tag_module *const initrd = get_ramdisk(mboot_header_phys_addr);
    const char *const start_of_data = (const char*) GENERAL_MEM_P2V(initrd->mod_start);
    serial_write(start_of_data, initrd->mod_end - initrd->mod_start);

    halt();
}
