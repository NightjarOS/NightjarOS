#pragma once

#include <stdint.h>

#include <kernel/boot/multiboot.h>

#include <libc/required_libc_functions.h>

#include <kernel/mem/mem_constants.h>

#include <kernel/drivers/serial/serial.h>
#include <kernel/error/error.h>

// NOTE: Some of these structures are already naturally aligned and thus the packed attribute is unneeded,
//  but it's harmless to include it anyways, so we just make them all packed to avoid accidentally not packing one that needs to be.

// Root System Description Pointer
struct RSDP {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision; // Has a value of 0 in ACPI 1.0 and has a value of 2 in ACPI 2.0 and later
    uint32_t RsdtAddress; // deprecated since ACPI 2.0

    // ACPI 2.0+ below:
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t Reserved[3];
} __attribute__ ((packed));

// System Description Table Header
struct SDT {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
} __attribute__ ((packed));

// Extended System Description Table
struct XSDT {
    struct SDT header;
    uint64_t ptrsToOtherSDTs[]; // len = (header.Length - sizeof(struct SDT)) / sizeof(uint64_t)
} __attribute__ ((packed));

// Generic Address Structure
struct GAS {
    uint8_t AddressSpaceID;
    uint8_t RegisterWidth;
    uint8_t RegisterOffset;
    uint8_t AccessSize;
    uint64_t Address;
} __attribute__ ((packed));

// Fixed ACPI Description Table
struct FADT {
    struct SDT header;
    uint32_t FIRMWARE_CTRL;
    uint32_t DSDT;
    uint8_t Reserved;
    uint8_t Preferred_PM_Profile;
    uint16_t SCI_INT;
    uint32_t SMI_CMD;
    uint8_t ACPI_ENABLE;
    uint8_t ACPI_DISABLE;
    uint8_t S4BIOS_REQ;
    uint8_t PSTATE_CNT;
    uint32_t PM1a_EVT_BLK;
    uint32_t PM1b_EVT_BLK;
    uint32_t PM1a_CNT_BLK;
    uint32_t PM1b_CNT_BLK;
    uint32_t PM2_CNT_BLK;
    uint32_t PM_TMR_BLK;
    uint32_t GPE0_BLK;
    uint32_t GPE1_BLK;
    uint8_t PM1_EVT_LEN;
    uint8_t PM1_CNT_LEN;
    uint8_t PM2_CNT_LEN;
    uint8_t PM_TMR_LEN;
    uint8_t GPE0_BLK_LEN;
    uint8_t GPE1_BLK_LEN;
    uint8_t GPE1_BASE;
    uint8_t CST_CNT;
    uint16_t P_LVL2_LAT;
    uint16_t P_LVL3_LAT;
    uint16_t FLUSH_SIZE;
    uint16_t FLUSH_STRIDE;
    uint8_t DUTY_OFFSET;
    uint8_t DUTY_WIDTH;
    uint8_t DAY_ALRM;
    uint8_t MON_ALRM;
    uint8_t CENTURY;
    uint16_t IAPC_BOOT_ARCH;
    uint8_t Reserved2;
    uint32_t Flags;
    struct GAS RESET_REG;
    uint8_t RESET_VALUE;
    uint16_t ARM_BOOT_ARCH;
    uint8_t FADTMinorVersion;
    uint64_t X_FIRMWARE_CTRL;
    uint64_t X_DSDT;
    struct GAS X_PM1a_EVT_BLK;
    struct GAS X_PM1b_EVT_BLK;
    struct GAS X_PM1a_CNT_BLK;
    struct GAS X_PM1b_CNT_BLK;
    struct GAS X_PM2_CNT_BLK;
    struct GAS X_PM_TMR_BLK;
    struct GAS X_GPE0_BLK;
    struct GAS X_GPE1_BLK;
    struct GAS SLEEP_CONTROL_REG;
    struct GAS SLEEP_STATUS_REG;
    uint64_t HypervisorVendorIdentity;
} __attribute__ ((packed));

struct InterruptEntryHeader {
    uint8_t Type; // support types [0-5] and [9-10]; all others are not relevant for x86_64 architecture
    uint8_t Length;
} __attribute__ ((packed));

// Multiple APIC Description Table
struct MADT {
    struct SDT header;
    uint32_t LocalInterruptControllerAddress;
    uint32_t Flags;
    struct InterruptEntryHeader InterruptControllerStructure[];
} __attribute__ ((packed));

enum LocalAPIC_Flags {
    Enabled = 1,
    OnlineCapable = 1 << 1,
};

enum MPS_INTI_Flags {
    Polarity = 3,
    TriggerMode = 3 << 2,
};

// type 0
struct ProcessorLocal_APIC_Structure {
    struct InterruptEntryHeader header;
    uint8_t ACPI_ProcessorUID;
    uint8_t APIC_ID;
    uint32_t Flags;
} __attribute__ ((packed));

// type 1
struct IO_APIC_Structure {
    struct InterruptEntryHeader header;
    uint8_t IO_APIC_ID;
    uint8_t Reserved;
    uint32_t IO_APIC_Address;
    uint32_t GlobalSystemInterruptBase;
} __attribute__ ((packed));

// type 2
struct InterruptSourceOverrideStructure {
    struct InterruptEntryHeader header;
    uint8_t Bus;
    uint8_t Source;
    uint32_t GlobalSystemInterrupt;
    uint16_t Flags;
} __attribute__ ((packed));

// type 3
struct NMI_SourceStructure {
    struct InterruptEntryHeader header;
    uint16_t Flags;
    uint32_t GlobalSystemInterrupt;
} __attribute__ ((packed));

// type 4
struct LocalAPIC_NMI_Structure {
    struct InterruptEntryHeader header;
    uint8_t ACPI_ProcessorUID;
    uint16_t Flags;
    uint8_t LocalAPIC_LINT_Num;
} __attribute__ ((packed));

// type 5
struct LocalAPIC_AddressOverrideStructure {
    struct InterruptEntryHeader header;
    uint16_t Reserved;
    uint64_t LocalAPIC_Address;
} __attribute__ ((packed));

// type 9
struct ProcessorLocalx2APIC_Structure {
    struct InterruptEntryHeader header;
    uint16_t Reserved;
    uint32_t X2APIC_ID;
    uint32_t Flags;
    uint32_t ACPI_ProcessorUID;
} __attribute__ ((packed));

// type 10
struct Localx2APIC_NMI_Structure {
    struct InterruptEntryHeader header;
    uint16_t Flags;
    uint32_t ACPI_ProcessorUID;
    uint8_t Localx2APIC_LINT_Num;
    uint8_t Reserved[3];
} __attribute__ ((packed));






const struct RSDP* get_rsdp(const uint64_t mboot_header_phys_addr);
const struct XSDT* get_XSDT(const struct RSDP *const RSDP_virt_addr);
void enumerate_sdt_entries(const struct XSDT *const XSDT_virt_addr);
const struct FADT* get_FADT(const struct XSDT *const XSDT_virt_addr);
const struct MADT* get_MADT(const struct XSDT *const XSDT_virt_addr);
void enumerate_madt_interrupt_entries(const struct MADT *const MADT_virt_addr);
