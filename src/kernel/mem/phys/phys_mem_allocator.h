#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <libc/required_libc_functions.h>
#include <kernel/error/error.h>

#include <kernel/mem/mem_constants.h>

void phys_mem_alloc_init(uint64_t* metadata, uint64_t number_of_uint64t_entries);

void phys_mem_reserve_pages(uint64_t first_page_addr, uint64_t sizeof_region_to_reserve);

uint64_t phys_mem_allocate_page(void);
void phys_mem_free_page(uint64_t page_addr);
