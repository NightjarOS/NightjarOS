; bit layout for 4-level/5-level paging entries:
; bit 0 = present bit
; bit 1 = r/w bit
; bit 2 = user bit
; bit 3 = page level write-through bit
; bit 4 = page-level cache disable bit
; bit 5 = accessed bit
; bit 6 = ignored
; bit 7 = reserved (must be 0)
; bits [8, 10] = ignored
; bit 11 = ignored for normal paging, only used for HLAT paging (which we don't use)
; bits [12, 51] = physical address of 4KiB aligned page directory pointer
; bits [52, 62] = ignored
; bit 63 = execute-disable bit

SIZEOF_PAGE_TABLE equ 4096

PT_PRESENT equ 1
PT_WRITEABLE equ 2

ENTRIES_PER_PT equ 512
SIZEOF_PT_ENTRY equ 8
PAGE_SIZE equ 4096
STACK_SIZE equ 32 * PAGE_SIZE

; bit layout for cr4:
; bit 0 = virtual-8086 mode extensions. Enables interrupt-handling and exception-handling extensions in virtual-8086 mode when set; disables the extensions when clear. See the Intel manual for more information.
; bit 1 = protected-mode virtual interrupts. Enable hardware support for a virtual interrupt flag (VIF) in protected mode when set; disables the VIF in protected mode when clear.
; bit 2 = time stamp disable. Restricts the execution of the RDTSC instruction to ring 0 when set, allows it to run at any privilege level when clear.
; bit 3 = debugging extensions. Controls whether referencing debug registers DR4 and DR5 cause an undefined opcode exception to be generated.
; bit 4 = page size extensions. Enables 4MiB pages with 32-bit paging when set; restricts 32-bit paging to pages of 4KiB when clear.
; bit 5 = physical address extension. When set, enables paging to produce physical addresses with more than 32 bits. When clear, restricts physical addresses to 32 bits. Must be set before entering IA-32e mode.
; bit 6 = enables the machine-check exception when set.
; bit 7 = page global enable. Enables "global pages" when set, which have to do with the "global flag"/bit 8 in a page-directory-pointer-table entry.
; bit 8 = performance-monitoring counter enable. Enables execution of the RDPMC instruction for any protection level when set, execution of RDPMC is only allowed in ring 0 when clear.
; bit 9 = operating system support for FXSAVE and FXRSTOR instructions (OSFXSR).
; bit 10 = operating system support for unmasked SIMD floating-point exceptions (OSXMMEXCPT).
; bit 11 = user-mode instruction prevention. When set, the following instructions cannot be executed in ring > 0: SGDT, SIDT, SLDT, SMSW, and STR. Any attempts will cause a General Protection Fault.
; bit 12 = 57-bit linear addresses. When set in IA-32e mode, the processor uses 5-level paging to translate 57-bit linear addresses. When clear in IA-32e mode, the processor uses 4-level paging to translate 48-bit linear addresses. This bit cannot be modified in IA-32e mode.
; bit 13 = VMX-enable bit. Enables Virtual Machine Extensions operation when set.
; bit 14 = SMX-enable bit. Enables Safer Mode Extensions operation when set.
; bit 15 = reserved
; bit 16 = FSGSBASE-Enable bit. Enables the instructions RDFSBASE, RDGSBASE, WRFSBASE, WRGSBASE.
; bit 17 = PCID-enable bit. Enables process-context identifiers (PCIDs) when set.
; bit 18 = XSAVE and processor extended states-enable bit. Enables XGETBV, XSAVE, and XSTOR among other things.
; bit 19 = key-locker-enable bit. When set, the LOADIWKEY instruction is enabled and other things related to AES key locker.
; bit 20 = enables supervisor-mode execution prevention (SMEP) when set.
; bit 21 = enables supervisor-mode access prevention (SMAP) when set.
; bit 22 = enable protection keys for user-mode pages. See Intel manual and bit 24 for more information.
; bit 23 = control flow enforcement technology. This can only be set if cr0 bit 16 has been set.
; bit 24 = enable protection keys for supervisor-mode pages. 4-level and 5-level paging associate each supervisor-mode linear address with a protection key. When set, this flag allows use of the IA32_PKRS MSR to specify, for each protection key, whether supervisor-mode linear addresses with that protection key can be read or written.
; bit 25 = user-interrupts enable bit. Enables user-interrupts when set, including user-interrupt delivery, user-interrupt notification identification, and user-interrupt instructions.
; bit 26 = reserved
; bit 27 = linear-address space separation. When set, enables linear-address space separation.
; bit 28 = supervisor LAM enable. When set, enables linear-address masking for supervisor pointers
; bits [29, 31] = reserved

; bit layout for cr3:
; bits [0, 2] = reserved
; bit 3 = 
; bit 4 = 
; bits [5, 11] = reserved
; bits [12, 60] =
; bit 61 = LAM57. When set, enables masking of linear-address bits [57, 62]. Overrides cr3 bit 62.
; bit 62 = LAM48. When set and cr3 bit 61 is clear, enables masking of linear-address bits [48, 62].
; bit 63 = undefined/reserved (must be 0)

; bit layout for cr0:
; bit 0 = protection enable. Enables protection mode when set. This flag does not enable paging directly. It only enables segment-level protection. Both this and bit 31 must be set to enable paging.
; bit 1 = monitor coprocessor. Controls instruction interactions regarding bit 3.
; bit 2 = emulation bit. Indicates the processor does not have an x87 when set, indicates it does when clear.
; bit 3 = task switched. Allows saving context on a task switch to be delayed until an instruction is actually executed by the new task.
; bit 4 = extension type. Indicates support for x87 math coprocessor when set.
; bit 5 = numeric error (for error reporting for x87 FPU)
; bits [6, 15] = reserved
; bit 16 = write protect (when set, it enforces page write protections in supervisor/ring 0 mode). This bit must be set before setting cr4 bit 23, and it cannot be cleared until cr4 bit 23 has been cleared.
; bit 17 = reserved
; bit 18 = alignment mask
; bits [19, 28] = reserved
; bit 29 = not write-through
; bit 30 = cache disable. When bits 30 and 29 are clear, cache is enabled.
; bit 31 = enables paging when set (bit 0 must also be set first).

; bit layout for IA32_EFER:
; bit 0 = SYSCALL Enable: IA32_EFER.SCE (R/W). Enable SYSCALL/SYSRET instructions in 64-bit mode.
; bits [1, 7] = reserved
; bit 8 = IA-32e Mode Enable: IA32_EFER.LME (R/W). Enables IA-32e mode operation.
; bit 9 = reserved
; bit 10 = IA-32e Mode Active: IA32_EFER.LMA (R). Indicates IA-32e mode is active when set.
; bit 11 = Execute Disable Bit Enable: IA32_EFER.NXE (R/W). Enables page access restriction by preventing instruction fetches from PAE pages with the XD bit set.
; bits [12, 63] = reserved

; bit layout for EFLAGS register:
; bit 0 = carry flag
; bit 1 = reserved (must be 1)
; bit 2 = parity flag
; bit 3 = reserved (must be 0)
; bit 4 = auxillary carry/adjust flag
; bit 5 = reserved (must be 0)
; bit 6 = zero flag
; bit 7 = sign flag
; bit 8 = trap bit. Set to enable single-step mode for debugging; clear to disable it.
; bit 9 = interrupt enable. Controls the reponse of the processor to maskable hardware interrupt requests. This flag is set to respond to maskable hardware interrupts; cleared to inhibit maskable hardware interrupts.
; bit 10 = direction flag
; bit 11 = overflow flag
; bit 12 and 13: Indicates the I/O privilege level of the currently running program or task. Related to POPF and IRET.
; bit 14 = nested task. Controls the chaining of interrupted and called tasks.
; bit 15 = reserved (must be 0)
; bit 16 = resume. Controls the processor's response to instruction-breakpoint conditions.
; bit 17 = virtual-8086 mode. Set to enable virtual-8086 mode; clear to return to protected mode.
; bit 18 = alignnment check or access control. Related to cr0 bit 18.
; bit 19 = virtual interrupt. Contains the virtual image of the IF flag. This is used in conjunction with cr4 bit 1.
; bit 20 = virtual interrupt pending. Set by software to indicate a virtual interrupt is pending; cleared to indicate no interrupt is pending.
; bit 21 = identification. The ability of a program to set or clear this flag indicates support for the CPUID instruction.
; bit [22, 31] = reserved (must be 0)

CR0_PAGING equ 1 << 31

EFLAGS_ID_BIT equ 1 << 21

CPUID_EXTENSIONS equ 0x80000000
CPUID_EXTENSIONS_FEATURES equ 0x80000001
CPUID_EDX_EXTENSION_FEATURE_LONG_MODE equ 1 << 29

PAE_ENABLE_BIT equ 1 << 5
MACHINE_CHECK_ENABLE_BIT equ 1 << 6
GLOBAL_PAGE_ENABLE_BIT equ 1 << 7

IA32_EFER equ 0xC0000080
IA32_EFER_LME_BIT equ 1 << 8

PAGING_ENABLE_BIT equ 1 << 31

KERNEL_OFFSET equ 0xFFFFFFFF80000000

%define V2P(a) ((a) - KERNEL_OFFSET)

MULTIBOOT2_HEADER_MAGIC equ 0xe85250d6
GRUB_MULTIBOOT_ARCHITECTURE_I386 equ 0
MULTIBOOT_HEADER_TAG_FRAMEBUFFER equ 5
MULTIBOOT_HEADER_TAG_OPTIONAL equ 1
MULTIBOOT_HEADER_TAG_END equ 0


section .multiboot
align 8
multiboot_header:
    dd MULTIBOOT2_HEADER_MAGIC
    dd GRUB_MULTIBOOT_ARCHITECTURE_I386
    dd multiboot_header_end - multiboot_header
    dd -(MULTIBOOT2_HEADER_MAGIC + GRUB_MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header))

framebuffer_tag_start:  
    dw MULTIBOOT_HEADER_TAG_FRAMEBUFFER
    dw MULTIBOOT_HEADER_TAG_OPTIONAL
    dd framebuffer_tag_end - framebuffer_tag_start
    dd 1024
    dd 768
    dd 32
framebuffer_tag_end:

align 8
end_tag:
    dw MULTIBOOT_HEADER_TAG_END
    dw 0
    dd 8
multiboot_header_end:


section .bss

global boot_stack
BITS 64
align PAGE_SIZE
resb STACK_SIZE ; 32KiB kernel stack size
boot_stack:

global pml4t
BITS 64
align PAGE_SIZE
pml4t:
resb PAGE_SIZE
pdpt:
resb PAGE_SIZE
pdt:
resb PAGE_SIZE
pt:
resb PAGE_SIZE


section .boot_trampoline

BITS 32
boot_die:
    ; TODO: In the future, we should print an error message instead of just silently `hlt`ing.
    hlt

BITS 32
disable_paging_32_bit:
    mov ecx, cr0 ; we use `ecx` since we call this before we set up a stack and the multiboot magic is passed to us in `eax`
    and ecx, ~CR0_PAGING
    mov cr0, ecx
    ret

BITS 32
check_cpuid:
    pushfd
    pop eax

    mov ecx, eax ; store the original EFLAGS content so it can be compared
    xor eax, EFLAGS_ID_BIT ; toggles bit 21 of cpuid

    ; if setting the new toggled EFLAGS and then reading it back gives us the toggled version, then CPUID is supported
    push eax
    popfd
    pushfd
    pop eax

    ; restore original untoggled EFLAGS
    push ecx
    popfd

    ; check if it is the toggled or untoggled EFLAGS register
    xor eax, ecx
    jz .not_supported
    ret
    .not_supported:
        jmp boot_die

BITS 32
check_support_for_long_mode:
    xor ecx, ecx
    mov eax, CPUID_EXTENSIONS
    cpuid

    xor ecx, ecx
    cmp eax, CPUID_EXTENSIONS_FEATURES
    jb .no_long_mode ; Intel manual says that eax >= CPUID_EXTENSIONS_FEATURES if any of the CPUID extensions are supported

    xor ecx, ecx
    mov eax, CPUID_EXTENSIONS_FEATURES
    cpuid

    test edx, CPUID_EDX_EXTENSION_FEATURE_LONG_MODE
    jz .no_long_mode ; check if long mode support bit is set
    ret
    .no_long_mode:
        jmp boot_die

global _start
BITS 32
_start:
    cli

    call disable_paging_32_bit

    mov esp, V2P(boot_stack)

    push ebx ; mboot header
    push eax ; mboot magic

    extern multiboot_total_size
    mov ecx, [ebx] ; We read this here since paging is disabled. This is more elegant than mapping a page just to read the multiboot size (and then mapping the rest afterwards).
    mov [V2P(multiboot_total_size)], ecx

    lgdt [V2P(gdt_ptr)]

    jmp 0x08:.load_segments
.load_segments:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    call check_cpuid
    call check_support_for_long_mode

    ; this is optional and sets up the Page Attribution Table (PAT) as follows:
    ; PAT0 = 0x06 (Write-back)
    ; PAT1 = 0x04 (Write-through)
    ; PAT2 = 0x00 (Uncached)
    ; PAT3 = 0x00 (Uncached)
    ; PAT4 = 0x00 (Uncached)
    ; PAT5 = 0x01 (Write-Combining)
    ; PAT6 = 0x00 (Uncached)
    ; PAT7 = 0x00 (Uncached)     
    mov eax, 0x00000406
	mov edx, 0x00000100
	mov ecx, 0x277
	wrmsr

    mov eax, cr4
    or eax, PAE_ENABLE_BIT | MACHINE_CHECK_ENABLE_BIT | GLOBAL_PAGE_ENABLE_BIT
    mov cr4, eax

    mov ecx, IA32_EFER
    rdmsr
    or eax, IA32_EFER_LME_BIT
    wrmsr

    ; zero out the page tables:
    mov edi, V2P(pml4t)
    xor eax, eax
    mov ecx, SIZEOF_PAGE_TABLE/4
    rep stosd
    ; Equivalent C code:
    ; uint32_t* edi = V2P(pml4t);
    ; uint32_t eax = 0u;
    ; uint32_t ecx = SIZEOF_PAGE_TABLE;
    ; for(int i = 0; i < ecx; ++i) {
    ;     *edi = eax;
    ;     edi += 1; // increments the address by 4 since it is a pointer to a 4-byte value
    ; }
    ; We repeat this `SIZEOF_PAGE_TABLE` number of times since we are overwriting 4 pages worth of data (for each hierarchy of the 4-level page table)

    ; TODO: Currently we hardcode in/assume that the kernel image size is <= 2MiB - 1MiB = 1MiB (we map 2MiB, but we minus the 1MiB because we skip it with our `KERNEL_PHYS_START` in the linker script).
    ;   In the future, we should instead have linker symbols that let us know the size of the kernel and then use that info to always map the necessary number of pages.
    ;   But because there is no harm in mapping a bit more low RAM than the kernel needs and because for the near future our kernel size will be <= 1MiB, this is fine as-is.
    ;   We will also have to change this if we ever do KASLR.

    ; set up page directory structure:
    mov edi, V2P(pml4t)
    mov eax, V2P(pdpt)
    or eax, PT_PRESENT | PT_WRITEABLE
    mov DWORD [edi], eax ; identity map
    add edi, 511 * SIZEOF_PT_ENTRY
    mov DWORD [edi], eax ; higher half map

    mov edi, V2P(pdpt)
    mov eax, V2P(pdt)
    or eax, PT_PRESENT | PT_WRITEABLE
    mov DWORD [edi], eax ; identity map
    add edi, 510 * SIZEOF_PT_ENTRY
    mov DWORD [edi], eax ; higher half map

    mov edi, V2P(pdt)
    mov eax, V2P(pt)
    or eax, PT_PRESENT | PT_WRITEABLE
    mov DWORD [edi], eax
    ; since the higher half map offset is 2MiB aligned, the PDT entry is the same for both the identity map and higher half

    ; fill up the page table:
    mov edi, V2P(pt)
    mov ebx, PT_PRESENT | PT_WRITEABLE
    mov ecx, ENTRIES_PER_PT

.set_entry:
    mov DWORD [edi], ebx
    add ebx, PAGE_SIZE
    add edi, SIZEOF_PT_ENTRY
    loop .set_entry
    ; Equivalent C code:
    ; uint32_t* edi = PT_ADDR;
    ; uint32_t ebx = PT_PRESENT | PT_WRITEABLE;
    ; for(uint32_t ecx = ENTRIES_PER_PT; ecx != 0; --ecx) {
    ;     *edi = ebx;
    ;     ebx += PAGE_SIZE;
    ;     edi += SIZEOF_PT_ENTRY/4; // divided by 4 since `edi` is a pointer to a 4-byte value (so `+= 1` increments the address by 4)
    ; }

    mov edi, V2P(pml4t)
    mov cr3, edi

    mov eax, cr0
    or eax, PAGING_ENABLE_BIT
    mov cr0, eax

    pop ecx ; mboot magic
    pop ebx ; mboot header

    jmp 0x18:V2P(long_mode_entry)

section .text

BITS 64
long_mode_entry:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, boot_stack

    ; We have to indirect jump here so that it will actually load the higher half absolute address.
    ;  If we direct jump, because both `long_mode_entry` and `higher_half_long_mode` are in the `.text` section,
    ;   it'll just do a relative jump and not actually load us into the higher half address space.
    mov rax, higher_half_long_mode
    jmp rax

BITS 64
higher_half_long_mode:
    mov rdi, pml4t
    mov QWORD [rdi], 0 ; remove identity mapping

    mov rdi, pdpt ; use the higher half virtual address since we just removed the identity map entry
    mov QWORD [rdi], 0 ; remove identity mapping

    mov rdi, V2P(pml4t)
    mov cr3, rdi

    mov rdi, rcx    ; first arg = mboot magic
    mov rsi, rbx    ; second arg = mboot header
    extern kernel_main
    call kernel_main


section .data

align 16
gdt_start:
; null segment:
    dd 0x00000000
    dd 0x00000000
; 32 bit code segment:
    dd 0x0000FFFF
    dd 0x00CF9A00
; 32 bit data segment:
    dd 0x0000FFFF
    dd 0x00CF9200
; 64 bit code segment:
    dd 0x00000000
    dd 0x00AF9A00
gdt_end:

align 4
gdt_ptr:
    dw gdt_end - gdt_start - 1 ; sizeof(gdt) - 1
    dd V2P(gdt_start)
