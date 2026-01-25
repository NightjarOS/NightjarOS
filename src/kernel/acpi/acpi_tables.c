#include "acpi_tables.h"


const struct RSDP* get_rsdp(const uint64_t mboot_header_phys_addr) {
    uint64_t current_phys_ptr = mboot_header_phys_addr + 2*sizeof(multiboot_uint32_t);

    for(;;) {
        const struct multiboot_tag *const current_virt_ptr = (struct multiboot_tag*) GENERAL_MEM_P2V(current_phys_ptr);

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_END) break;

        if(current_virt_ptr->type == MULTIBOOT_TAG_TYPE_ACPI_NEW) {
            const struct multiboot_tag_new_acpi *const acpi_tag = (struct multiboot_tag_new_acpi*) current_virt_ptr;
            return (const struct RSDP*) acpi_tag->rsdp;
        }

        current_phys_ptr = (uint64_t)(current_phys_ptr  + round_up(current_virt_ptr->size, MULTIBOOT_TAG_ALIGN));
    }

    halt_and_die("RSDP not found.");
}

const struct XSDT* get_XSDT(const struct RSDP *const RSDP_virt_addr) {
    kassert(RSDP_virt_addr != NULL, "RSDP is null.");

    if(strncmp(RSDP_virt_addr->Signature, "RSD PTR ", 8) != 0) {
        halt_and_die("Invalid RSDP.");
    }
    if(RSDP_virt_addr->Revision != 2) {
        halt_and_die("ACPI version is too old.");
    }
    kassert(RSDP_virt_addr->Length == sizeof(struct RSDP), "Invalid RSDP size.");
    // TODO: Also calculate the checksum

    return (const struct XSDT*) GENERAL_MEM_P2V(RSDP_virt_addr->XsdtAddress);
}

void enumerate_sdt_entries(const struct XSDT *const XSDT_virt_addr) {
    const uint64_t number_of_SDTs = (XSDT_virt_addr->header.Length - sizeof(struct SDT)) / sizeof(uint64_t);

    serial_writestring("SDT entries:\n");
    for(uint64_t i = 0; i < number_of_SDTs; ++i) {
        const struct SDT *const sdt_header = (const struct SDT*) GENERAL_MEM_P2V(XSDT_virt_addr->ptrsToOtherSDTs[i]);
        serial_write(sdt_header->Signature, 4);
        serial_writestring("\n");
    }
}

const struct FADT* get_FADT(const struct XSDT *const XSDT_virt_addr) {
    const uint64_t number_of_SDTs = (XSDT_virt_addr->header.Length - sizeof(struct SDT)) / sizeof(uint64_t);

    for(uint64_t i = 0; i < number_of_SDTs; ++i) {
        const struct SDT *const sdt_header = (const struct SDT*) GENERAL_MEM_P2V(XSDT_virt_addr->ptrsToOtherSDTs[i]);
        if(strncmp(sdt_header->Signature, "FACP", 4) == 0) {
            return (const struct FADT*) sdt_header;
        }
    }

    halt_and_die("No FADT found.");
}

const struct MADT* get_MADT(const struct XSDT *const XSDT_virt_addr) {
    const uint64_t number_of_SDTs = (XSDT_virt_addr->header.Length - sizeof(struct SDT)) / sizeof(uint64_t);

    for(uint64_t i = 0; i < number_of_SDTs; ++i) {
        const struct SDT *const sdt_header = (const struct SDT*) GENERAL_MEM_P2V(XSDT_virt_addr->ptrsToOtherSDTs[i]);
        if(strncmp(sdt_header->Signature, "APIC", 4) == 0) {
            return (const struct MADT*) sdt_header;
        }
    }

    halt_and_die("No MADT found.");
}

static const char* get_name_of_madt_interrupt_entry_type(const uint8_t type) {
    switch(type) {
        case 0:
            return "ProcessorLocal_APIC_Structure";
        case 1:
            return "IO_APIC_Structure";
        case 2:
            return "InterruptSourceOverrideStructure";
        case 3:
            return "NMI_SourceStructure";
        case 4:
            return "LocalAPIC_NMI_Structure";
        case 5:
            return "LocalAPIC_AddressOverrideStructure";
        case 9:
            return "ProcessorLocalx2APIC_Structure";
        case 10:
            return "Localx2APIC_NMI_Structure";
    }
    return "Unknown";
}

void enumerate_madt_interrupt_entries(const struct MADT *const MADT_virt_addr) {
    const uint64_t total_len = MADT_virt_addr->header.Length;
    uint64_t offset = sizeof(struct MADT);
    const uint8_t* ptr = (const uint8_t*)MADT_virt_addr->InterruptControllerStructure;

    serial_writestring("enumerate_madt_interrupt_entries:\n");
    while(offset + sizeof(struct InterruptEntryHeader) <= total_len) {
        const struct InterruptEntryHeader *const current_interrupt_header = (const struct InterruptEntryHeader*) ptr;

        if (current_interrupt_header->Length < sizeof(struct InterruptEntryHeader)) {
            halt_and_die("MADT entry has invalid Length.");
        }

        if (offset + current_interrupt_header->Length > total_len) {
            halt_and_die("MADT entry overruns table length.");
        }

        serial_writestring("Type: ");
        serial_writestring(get_name_of_madt_interrupt_entry_type(current_interrupt_header->Type));
        serial_writestring("\n");

        const uint64_t entry_len = current_interrupt_header->Length;
        offset += entry_len;
        ptr += entry_len;
    }
}

