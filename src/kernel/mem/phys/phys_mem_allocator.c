#include "phys_mem_allocator.h"

static uint64_t* phys_mem_meta_data;
static uint64_t phys_mem_number_of_uint64t_entries; // if the total number of pages is not a multiple of 64, we just reserve the excess bits in the last entry using `reserve_page()`

void phys_mem_alloc_init(uint64_t *const metadata, const uint64_t number_of_uint64t_entries) {
    phys_mem_meta_data = metadata;
    phys_mem_number_of_uint64t_entries = number_of_uint64t_entries;
    memset(phys_mem_meta_data, 0, phys_mem_number_of_uint64t_entries*sizeof(uint64_t));
}

static void set_bit(const uint64_t bit_index, const bool value) {
    if(value) {
        phys_mem_meta_data[bit_index/64ULL] |= 1ULL << (bit_index%64ULL);
    }
    else {
        phys_mem_meta_data[bit_index/64ULL] &= ~(1ULL << (bit_index%64ULL));
    }
}

static uint64_t get_bit(const uint64_t bit_index) {
    return phys_mem_meta_data[bit_index/64ULL] & (1ULL << (bit_index%64ULL));
}

static bool is_bit_set(const uint64_t bit_index) {
    return get_bit(bit_index) > 0u;
}

void phys_mem_reserve_pages(const uint64_t first_page_addr, const uint64_t sizeof_region_to_reserve) {
    kassert(phys_mem_meta_data != NULL, "phys_mem_alloc_init() was not called.");

    if(sizeof_region_to_reserve == 0ULL) return;

    const uint64_t aligned_first_page_addr = round_down_to_page(first_page_addr);
    const uint64_t aligned_sizeof_to_reserve = round_down_to_page(first_page_addr + sizeof_region_to_reserve - 1ULL) - aligned_first_page_addr + NORMAL_PAGE_SIZE;

    uint64_t number_of_pages_to_reserve = aligned_sizeof_to_reserve/NORMAL_PAGE_SIZE;
    uint64_t current_page_to_reserve = aligned_first_page_addr/NORMAL_PAGE_SIZE;

    kassert((current_page_to_reserve + number_of_pages_to_reserve - 1ULL) < phys_mem_number_of_uint64t_entries*64ULL, "Reserve index is out of bounds.");

    while(number_of_pages_to_reserve != 0) {
        if(current_page_to_reserve % 64ULL != 0ULL) {
            kassert(is_bit_set(current_page_to_reserve) == false, "Reserving an already used page.");
            set_bit(current_page_to_reserve, 1);
            --number_of_pages_to_reserve;
            ++current_page_to_reserve;
        }
        else if(number_of_pages_to_reserve >= 64ULL) {
            kassert(phys_mem_meta_data[current_page_to_reserve/64ULL] == 0, "Reserving an already used page.");
            phys_mem_meta_data[current_page_to_reserve/64ULL] = (uint64_t) -1;
            number_of_pages_to_reserve -= 64ULL;
            current_page_to_reserve += 64ULL;
        }
        else {
            kassert(is_bit_set(current_page_to_reserve) == false, "Reserving an already used page.");
            set_bit(current_page_to_reserve, 1);
            --number_of_pages_to_reserve;
            ++current_page_to_reserve;
        }
    }
}

uint64_t phys_mem_allocate_page(void) {
    kassert(phys_mem_meta_data != NULL, "phys_mem_alloc_init() was not called.");

    for(uint64_t i = 0ULL; i < phys_mem_number_of_uint64t_entries; ++i) {
        if(phys_mem_meta_data[i] != (uint64_t) -1) {
            for(uint64_t j = 0ULL; j < 64ULL; ++j) {
                const uint64_t page_index = i*64ULL + j;
                if(is_bit_set(page_index) == false) {
                    set_bit(page_index, 1);
                    return page_index*NORMAL_PAGE_SIZE;
                }
            }
        }
    }
    halt_and_die("Out of physical memory.");
}

void phys_mem_free_page(const uint64_t page_addr) {
    kassert(phys_mem_meta_data != NULL, "phys_mem_alloc_init() was not called.");

    const uint64_t page_index = page_addr/NORMAL_PAGE_SIZE; // no need to `round_down_to_page()` since integer division already implicitly does this for us

    kassert(page_index < phys_mem_number_of_uint64t_entries*64ULL, "Free address is out of bounds.");

    kassert(is_bit_set(page_index) == true, "Freeing an already free physical page.");

    set_bit(page_index, 0);
}
