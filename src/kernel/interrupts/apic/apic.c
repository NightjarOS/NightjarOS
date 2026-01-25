#include "apic.h"

struct IDT_entry interrupt_descriptor_table[256];

void remap_and_mask_pic_interrupts(void) {
    
}

void idt_register_handler(const uint8_t interrupt, const uint32_t address) {
    
}

void idt_init(void) {
    if(!is_x2apic_supported()) {
        halt_and_die("x2apic is unsupported.");
    }

    enable_x2apic();

    mask_all_lvt_registers();

    set_timer_to_tsc_deadline_mode();

    write_deadline_value(TSC_DEADLINE_QUANTUM);

    struct descriptor_table_pseudo_register descriptor_table;
    descriptor_table.limit = sizeof(interrupt_descriptor_table) - 1;
    descriptor_table.base = (uint64_t) interrupt_descriptor_table;

    asm volatile("lidt %0" : : "m"(descriptor_table));
}

