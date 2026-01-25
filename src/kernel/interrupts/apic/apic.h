#pragma once

#include <stdint.h>
#include <stddef.h>

struct IDT_entry {
    uint16_t offset_lower_bits;
    uint16_t segment_selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid_bits;
    uint32_t offset_high_bits;
    uint32_t reserved;
} __attribute__ ((packed));

struct descriptor_table_pseudo_register {
    uint16_t limit;
    uint64_t base;
} __attribute__ ((packed));

// For now we are hardcoding our times based on a clock rate of 3GHz.
//  Thus, the below is equal to 1ms.
// TODO: We should use cpuid leaf 0x15 or fallback to HPET calibration to
//  determine the ticks to time rate (which varies by clock rate).
#define TSC_DEADLINE_QUANTUM 3000000ULL

// 1 = supported, 0 = unsupported
extern uint64_t is_x2apic_supported(void);
extern void enable_x2apic(void);

extern void mask_all_lvt_registers(void);
extern void unmask_used_lvt_registers(void);

extern void set_timer_to_tsc_deadline_mode(void);
extern void write_deadline_value(uint64_t delta_ticks);


