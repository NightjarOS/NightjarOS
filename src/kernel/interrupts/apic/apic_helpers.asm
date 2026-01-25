CPUID_ECX_FEATURE_X2APIC_MODE equ 1 << 21

IA32_APIC_BASE equ 0x1B

XAPIC_GLOBAL_ENABLE equ 1 << 11
X2APIC_ENABLE equ 1 << 10

LVT_CMCI_REGISTER equ 0x82F
LVT_TIMER_REGISTER equ 0x832
LVT_THERMAL_SENSOR_REGISTER equ 0x833 
LVT_PERFORMANCE_MONITORING_REGISTER equ 0x834
LVT_LINT0_REGISTER equ 0x835
LVT_LINT1_REGISTER equ 0x836
LVT_ERROR_REGISTER equ 0x837

MASKING_BIT equ 1 << 16

LVT_TIMER_MODE_MASK equ 3 << 17
LVT_TIMER_TSC_DEADLINE_MODE equ 2 << 17 ; write 10 to bits 18:17
IA32_TSC_DEADLINE_MSR_ADDRESS equ 0x6E0

section .text

global is_x2apic_supported
is_x2apic_supported:
    push rbx ; cpuid clobbers ebx, so we must save it

    xor ecx, ecx
    mov eax, 1
    cpuid

    test ecx, CPUID_ECX_FEATURE_X2APIC_MODE
    setnz al        ; AL = 1 if supported, 0 otherwise
    movzx rax, al   ; zero-extend to full RAX

    pop rbx
    ret

global enable_x2apic
enable_x2apic:
    ; we do not modify rbx, so don't need to save it

    mov ecx, IA32_APIC_BASE
    rdmsr
    or eax, XAPIC_GLOBAL_ENABLE | X2APIC_ENABLE
    wrmsr

    ret

global mask_all_lvt_registers
mask_all_lvt_registers:
    ; we do not modify rbx, so don't need to save it

    mov ecx, LVT_CMCI_REGISTER
    rdmsr
    or eax, MASKING_BIT
    wrmsr

    mov ecx, LVT_TIMER_REGISTER
    rdmsr
    or eax, MASKING_BIT
    wrmsr

    mov ecx, LVT_THERMAL_SENSOR_REGISTER
    rdmsr
    or eax, MASKING_BIT
    wrmsr

    mov ecx, LVT_PERFORMANCE_MONITORING_REGISTER
    rdmsr
    or eax, MASKING_BIT
    wrmsr

    mov ecx, LVT_LINT0_REGISTER
    rdmsr
    or eax, MASKING_BIT
    wrmsr

    mov ecx, LVT_LINT1_REGISTER
    rdmsr
    or eax, MASKING_BIT
    wrmsr

    mov ecx, LVT_ERROR_REGISTER
    rdmsr
    or eax, MASKING_BIT
    wrmsr

    ret

global unmask_used_lvt_registers
unmask_used_lvt_registers:
    ; we do not modify rbx, so don't need to save it

    mov ecx, LVT_TIMER_REGISTER
    rdmsr
    and eax, ~MASKING_BIT
    wrmsr

    mov ecx, LVT_ERROR_REGISTER
    rdmsr
    and eax, ~MASKING_BIT
    wrmsr

    ret

global set_timer_to_tsc_deadline_mode
set_timer_to_tsc_deadline_mode:
    ; we do not modify rbx, so don't need to save it

    mov ecx, LVT_TIMER_REGISTER
    rdmsr
    and eax, ~LVT_TIMER_MODE_MASK ; clear bits 18:17
    or eax, LVT_TIMER_TSC_DEADLINE_MODE ; set bits 18:17 to 10, which is the value for TSC-deadline
    wrmsr

    ret

global write_deadline_value
write_deadline_value:
    ; we do not modify rbx, so don't need to save it

    mov rdi, rax

    ; TODO: Look into using rdtscp, especially when we do SMP
    rdtsc ; EDX:EAX = current TSC

    mov rdi, TSC_DEADLINE_QUANTUM

    mov ecx, edi ; ECX = low 32 bits of delta
    shr rdi, 32 ; RDI = high 32 bits of delta

    add eax, ecx ; add low half
    adc edx, edi ; add high half + carry

    mov ecx, IA32_TSC_DEADLINE_MSR_ADDRESS
    wrmsr

    ret
