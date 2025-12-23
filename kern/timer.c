#include <inc/types.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/x86.h>
#include <inc/uefi.h>
#include <kern/timer.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define kilo      (1000ULL)
#define Mega      (kilo * kilo)
#define Giga      (kilo * Mega)
#define Tera      (kilo * Giga)
#define Peta      (kilo * Tera)
#define ULONG_MAX ~0UL

#if LAB <= 6
/* Early variant of memory mapping that does 1:1 aligned area mapping
 * in 2MB pages. You will need to reimplement this code with proper
 * virtual memory mapping in the future. */
void *
mmio_map_region(physaddr_t pa, size_t size) {
    void map_addr_early_boot(uintptr_t addr, uintptr_t addr_phys, size_t sz);
    const physaddr_t base_2mb = 0x200000;
    uintptr_t org = pa;
    size += pa & (base_2mb - 1);
    size += (base_2mb - 1);
    pa &= ~(base_2mb - 1);
    size &= ~(base_2mb - 1);
    map_addr_early_boot(pa, pa, size);
    return (void *)org;
}
void *
mmio_remap_last_region(physaddr_t pa, void *addr, size_t oldsz, size_t newsz) {
    return mmio_map_region(pa, newsz);
}
#endif

struct Timer timertab[MAX_TIMERS];
struct Timer *timer_for_schedule;

struct Timer timer_hpet0 = {
        .timer_name = "hpet0",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim0,
        .handle_interrupts = hpet_handle_interrupts_tim0,
};

struct Timer timer_hpet1 = {
        .timer_name = "hpet1",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim1,
        .handle_interrupts = hpet_handle_interrupts_tim1,
};

struct Timer timer_acpipm = {
        .timer_name = "pm",
        .timer_init = acpi_enable,
        .get_cpu_freq = pmtimer_cpu_frequency,
};

void
acpi_enable(void) {
    FADT *fadt = get_fadt();
    outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
    while ((inw(fadt->PM1aControlBlock) & 1) == 0) /* nothing */
        ;
}

bool
is_valid_checksum(uint8_t *ptr, uint32_t length) {
    uint32_t sum = 0;
    for (size_t i = 0; i < length; ++i) {
        sum += *ptr;
        ++ptr;
    }

    sum &= 0xFFU;

    return (sum == 0);
}

// LAB 5
RSDP *
get_rsdp() {
    RSDP *rsd_ptr = (RSDP *) mmio_map_region((physaddr_t) uefi_lp->ACPIRoot, sizeof(RSDP));

    const uint32_t RSDT_CHECKSUM_LENGHTH = 20;
    if (!is_valid_checksum((uint8_t *) rsd_ptr, RSDT_CHECKSUM_LENGHTH)) // 20?????????????????????????
    {
        panic("get_rsdp: invalid RSDP checksum\n");
    }

    if (strncmp(rsd_ptr->Signature, "RSD PTR ", sizeof(rsd_ptr->Signature))) {
        panic("get_rsdp: invalid RSDP signature\n");
    }

    return rsd_ptr;
}

// static void *
// acpi_find_table(const char *sign) {
//     /*
//      * This function performs lookup of ACPI table by its signature
//      * and returns valid pointer to the table mapped somewhere.
//      *
//      * It is a good idea to checksum tables before using them.
//      *
//      * HINT: Use mmio_map_region/mmio_remap_last_region
//      * before accessing table addresses
//      * (Why mmio_remap_last_region is requrired?)
//      * HINT: RSDP address is stored in uefi_lp->ACPIRoot
//      * HINT: You may want to distunguish RSDT/XSDT
//      */
//     // LAB 5: Your code here:
//
//     // https://wiki.osdev.org/ACPI
//     RSDP *rsd_ptr = get_rsdp();
//
//     RSDT *rsdt_ptr;    
//     if (rsd_ptr->Revision >= 2) {
//         rsdt_ptr = (RSDT *) mmio_map_region(
//             (physaddr_t) rsd_ptr->XsdtAddress, 
//             sizeof(RSDT)
//         );
//         
//         if (strncmp(rsdt_ptr->h.Signature, "XSDT", sizeof(rsdt_ptr->h.Signature))) {
//             panic("acpi_find_table: invalid XSDT signature\n");
//         }
//         
//         rsdt_ptr = (RSDT *) mmio_remap_last_region(
//             (physaddr_t) (rsd_ptr->XsdtAddress), 
//             (void *) (rsd_ptr->XsdtAddress),
//             sizeof(RSDT),
//             rsdt_ptr->h.Length
//         );
//     } else {
//         rsdt_ptr = (RSDT *) mmio_map_region(
//             (physaddr_t) (rsd_ptr->RsdtAddress),
//             sizeof(RSDT)
//         );
//         
//         if (strncmp(rsdt_ptr->h.Signature, "RSDT", sizeof(rsdt_ptr->h.Signature))) {
//             panic("acpi_find_table: invalid RSDT signature\n");
//         }
//         
//         rsdt_ptr = (RSDT *) mmio_remap_last_region(
//             (physaddr_t) (rsd_ptr->RsdtAddress), 
//             (void *) (uint64_t) (rsd_ptr->RsdtAddress),
//             sizeof(RSDT),
//             rsdt_ptr->h.Length
//         );
//     }
//
//     if (!is_valid_checksum((uint8_t *) rsdt_ptr, rsdt_ptr->h.Length)) {
//         panic("acpi_find_table: invalid RSDT/XSDT checksum\n");
//     }
//
//     // https://wiki.osdev.org/RSDT#Other_fields 4 and 8 constants
//     size_t sdt_num = rsdt_ptr->h.Length - sizeof(ACPISDTHeader);
//     if (rsd_ptr->Revision >= 2) {
//         sdt_num /= 8;
//     } else {
//         sdt_num /= 4;
//     }
//
//     for (size_t i = 0; i < sdt_num; ++i) {
//         ACPISDTHeader *hdr = (ACPISDTHeader *) mmio_map_region(
//             rsdt_ptr->PointerToOtherSDT[i], 
//             sizeof(ACPISDTHeader)
//         );
//
//         if (hdr == NULL/*???????????????????????????????????????????????????????!!!!!!!!!!!!!!!!!!!!*/)
//         {
//             // cprintf("acpi_find_table: no ACPISDTHeader");
//
//             continue;
//         }
//         
//         if (!strncmp(hdr->Signature, sign, sizeof(hdr->Signature))) {
//             if (!is_valid_checksum((uint8_t *) hdr, hdr->Length)) {
//                 panic("acpi_find_table: invalid %s header checksum\n", sign);
//             }
//
//             return hdr;
//         }
//     }
//
//     return NULL;
// }

static bool
acpi_checksum_ok(const void *addr, size_t len) {
    const uint8_t *p = (const uint8_t *)addr;
    uint8_t s = 0;
    for (size_t i = 0; i < len; i++)
        s = (uint8_t)(s + p[i]);
    return s == 0;
}

static ACPISDTHeader *
map_sdt_full(physaddr_t pa) {
    ACPISDTHeader *h = (ACPISDTHeader *)mmio_map_region(pa, sizeof(ACPISDTHeader));
    if (!h)
        return NULL;

    uint32_t len = h->Length;
    if (len < sizeof(ACPISDTHeader))
        return NULL;

    h = (ACPISDTHeader *)mmio_remap_last_region(pa, h, sizeof(ACPISDTHeader), len);
    if (!h)
        return NULL;

    if (!acpi_checksum_ok(h, len))
        return NULL;

    return h;
}

static void *
acpi_find_table(const char *sign) {
    // 1) Map RSDP
    physaddr_t rsdp_pa = (physaddr_t)uefi_lp->ACPIRoot;
    RSDP *rsdp = (RSDP *)mmio_map_region(rsdp_pa, sizeof(RSDP));
    if (!rsdp)
        return NULL;

    if (memcmp(rsdp->Signature, "RSD PTR ", 8) != 0)
        return NULL;

    // RSDP v1 checksum: first 20 bytes
    if (!acpi_checksum_ok(rsdp, 20))
        return NULL;

    // 2) Prefer XSDT if available (Revision >= 2)
    if (rsdp->Revision >= 2 && rsdp->XsdtAddress) {
        physaddr_t xsdt_pa = (physaddr_t)rsdp->XsdtAddress;
        ACPISDTHeader *xsdt = map_sdt_full(xsdt_pa);
        if (xsdt && memcmp(xsdt->Signature, "XSDT", 4) == 0) {
            uint32_t len = xsdt->Length;
            if (len >= sizeof(ACPISDTHeader)) {
                size_t n = (len - sizeof(ACPISDTHeader)) / 8;
                const uint8_t *ents = (const uint8_t *)xsdt + sizeof(ACPISDTHeader);

                for (size_t i = 0; i < n; i++) {
                    uint64_t entry;
                    memcpy(&entry, ents + i * 8, sizeof(entry));   // no misaligned load
                    physaddr_t pa = (physaddr_t)entry;

                    ACPISDTHeader *sdt = map_sdt_full(pa);
                    if (!sdt)
                        continue;

                    if (memcmp(sdt->Signature, sign, 4) == 0)
                        return sdt;
                }
            }
        }
    }

    // 3) Fallback to RSDT (32-bit entries)
    if (!rsdp->RsdtAddress)
        return NULL;

    physaddr_t rsdt_pa = (physaddr_t)rsdp->RsdtAddress;
    ACPISDTHeader *rsdt = map_sdt_full(rsdt_pa);
    if (!rsdt || memcmp(rsdt->Signature, "RSDT", 4) != 0)
        return NULL;

    uint32_t len = rsdt->Length;
    size_t n = (len - sizeof(ACPISDTHeader)) / 4;
    const uint8_t *ents = (const uint8_t *)rsdt + sizeof(ACPISDTHeader);

    for (size_t i = 0; i < n; i++) {
        uint32_t entry32;
        memcpy(&entry32, ents + i * 4, sizeof(entry32));        // safe even if packed
        physaddr_t pa = (physaddr_t)entry32;

        ACPISDTHeader *sdt = map_sdt_full(pa);
        if (!sdt)
            continue;

        if (memcmp(sdt->Signature, sign, 4) == 0)
            return sdt;
    }

    return NULL;
}


/* Obtain and map FADT ACPI table address. */
FADT *
get_fadt(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)
    // HINT: ACPI table signatures are
    //       not always as their names

    FADT *fadt_ptr = acpi_find_table("FACP");
    if (fadt_ptr == NULL) {
        panic("get_fadt: couldn't find FADT\n");
    }

    // fadt_ptr = (FADT *) mmio_remap_last_region(
    //     (physaddr_t) fadt_ptr,
    //     (void *) fadt_ptr,
    //     sizeof(ACPISDTHeader), 
    //     fadt_ptr->h.Length
    // );
    
    return fadt_ptr;
}

/* Obtain and map RSDP ACPI table address. */
HPET *
get_hpet(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)

    HPET *hpet_ptr = acpi_find_table("HPET");
    if (hpet_ptr == NULL) {
        panic("get_hpet: couldn't find HPET\n");
    }

    // hpet_ptr = (HPET *) mmio_remap_last_region(
    //     (physaddr_t) hpet_ptr,
    //     (void *) hpet_ptr,
    //     sizeof(ACPISDTHeader), 
    //     hpet_ptr->h.Length
    // );
    
    return hpet_ptr;
}

/* Getting physical HPET timer address from its table. */
HPETRegister *
hpet_register(void) {
    HPET *hpet_timer = get_hpet();
    if (!hpet_timer->address.address) panic("hpet is unavailable\n");

    uintptr_t paddr = hpet_timer->address.address;
    return mmio_map_region(paddr, sizeof(HPETRegister));
}

/* Debug HPET timer state. */
void
hpet_print_struct(void) {
    HPET *hpet = get_hpet();
    assert(hpet != NULL);
    cprintf("signature = %s\n", (hpet->h).Signature);
    cprintf("length = %08x\n", (hpet->h).Length);
    cprintf("revision = %08x\n", (hpet->h).Revision);
    cprintf("checksum = %08x\n", (hpet->h).Checksum);

    cprintf("oem_revision = %08x\n", (hpet->h).OEMRevision);
    cprintf("creator_id = %08x\n", (hpet->h).CreatorID);
    cprintf("creator_revision = %08x\n", (hpet->h).CreatorRevision);

    cprintf("hardware_rev_id = %08x\n", hpet->hardware_rev_id);
    cprintf("comparator_count = %08x\n", hpet->comparator_count);
    cprintf("counter_size = %08x\n", hpet->counter_size);
    cprintf("reserved = %08x\n", hpet->reserved);
    cprintf("legacy_replacement = %08x\n", hpet->legacy_replacement);
    cprintf("pci_vendor_id = %08x\n", hpet->pci_vendor_id);
    cprintf("hpet_number = %08x\n", hpet->hpet_number);
    cprintf("minimum_tick = %08x\n", hpet->minimum_tick);

    cprintf("address_structure:\n");
    cprintf("address_space_id = %08x\n", (hpet->address).address_space_id);
    cprintf("register_bit_width = %08x\n", (hpet->address).register_bit_width);
    cprintf("register_bit_offset = %08x\n", (hpet->address).register_bit_offset);
    cprintf("address = %08lx\n", (unsigned long)(hpet->address).address);
}

static volatile HPETRegister *hpetReg;
/* HPET timer period (in femtoseconds) */
static uint64_t hpetFemto = 0;
/* HPET timer frequency */
static uint64_t hpetFreq = 0;

/* HPET timer initialisation */
void
hpet_init() {
    if (hpetReg == NULL) {
        nmi_disable();
        hpetReg = hpet_register();
        uint64_t cap = hpetReg->GCAP_ID;
        hpetFemto = (uintptr_t)(cap >> 32);
        if (!(cap & HPET_LEG_RT_CAP)) panic("HPET has no LegacyReplacement mode");

        // cprintf("hpetFemto = %llu\n", hpetFemto);
        hpetFreq = (1 * Peta) / hpetFemto;
        // cprintf("HPET: Frequency = %d.%03dMHz\n", (uintptr_t)(hpetFreq / Mega), (uintptr_t)(hpetFreq % Mega));
        /* Enable ENABLE_CNF bit to enable timer */
        hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
        nmi_enable();
    }
}

/* HPET register contents debugging. */
void
hpet_print_reg(void) {
    cprintf("GCAP_ID = %016lx\n", (unsigned long)hpetReg->GCAP_ID);
    cprintf("GEN_CONF = %016lx\n", (unsigned long)hpetReg->GEN_CONF);
    cprintf("GINTR_STA = %016lx\n", (unsigned long)hpetReg->GINTR_STA);
    cprintf("MAIN_CNT = %016lx\n", (unsigned long)hpetReg->MAIN_CNT);
    cprintf("TIM0_CONF = %016lx\n", (unsigned long)hpetReg->TIM0_CONF);
    cprintf("TIM0_COMP = %016lx\n", (unsigned long)hpetReg->TIM0_COMP);
    cprintf("TIM0_FSB = %016lx\n", (unsigned long)hpetReg->TIM0_FSB);
    cprintf("TIM1_CONF = %016lx\n", (unsigned long)hpetReg->TIM1_CONF);
    cprintf("TIM1_COMP = %016lx\n", (unsigned long)hpetReg->TIM1_COMP);
    cprintf("TIM1_FSB = %016lx\n", (unsigned long)hpetReg->TIM1_FSB);
    cprintf("TIM2_CONF = %016lx\n", (unsigned long)hpetReg->TIM2_CONF);
    cprintf("TIM2_COMP = %016lx\n", (unsigned long)hpetReg->TIM2_COMP);
    cprintf("TIM2_FSB = %016lx\n", (unsigned long)hpetReg->TIM2_FSB);
}

/* HPET main timer counter value. */
uint64_t
hpet_get_main_cnt(void) {
    return hpetReg->MAIN_CNT;
}

/* - Configure HPET timer 0 to trigger every 0.5 seconds on IRQ_TIMER line
 * - Configure HPET timer 1 to trigger every 1.5 seconds on IRQ_CLOCK line
 *
 * HINT To be able to use HPET as PIT replacement consult
 *      LegacyReplacement functionality in HPET spec.
 * HINT Don't forget to unmask interrupt in PIC */
void
hpet_enable_interrupts_tim0(void) {
    // LAB 5: Your code here

    nmi_disable();
    {
        hpetReg->GEN_CONF |= HPET_LEG_RT_CNF; // turn on legacy mode

        hpetReg->TIM0_CONF |= HPET_TN_VAL_SET_CNF; // reset comparator value

        // comment if not working
        hpetReg->TIM0_CONF |= HPET_TN_TYPE_CNF; // enable periodic

        hpetReg->TIM0_CONF |= HPET_TN_INT_ENB_CNF; // turn on that timer

        hpetReg->TIM0_COMP = hpetFreq / 100000;
    }
    nmi_enable();

    pic_irq_unmask(IRQ_TIMER);
}

void
hpet_enable_interrupts_tim1(void) {
    // LAB 5: Your code here

    nmi_disable();
    {
        hpetReg->GEN_CONF |= HPET_LEG_RT_CNF; // turn on legacy mode

        hpetReg->TIM1_CONF |= HPET_TN_VAL_SET_CNF; // reset comparator value

        // comment if not working
        hpetReg->TIM1_CONF |= HPET_TN_TYPE_CNF; // enable periodic

        hpetReg->TIM1_CONF |= HPET_TN_INT_ENB_CNF; // turn on that timer

        hpetReg->TIM1_COMP = hpetFreq * 3 / 2;
    }
    nmi_enable();

    pic_irq_unmask(IRQ_CLOCK);
}

void
hpet_handle_interrupts_tim0(void) {
    pic_send_eoi(IRQ_TIMER);
}

void
hpet_handle_interrupts_tim1(void) {
    pic_send_eoi(IRQ_CLOCK);
}

/* Calculate CPU frequency in Hz with the help with HPET timer.
 * HINT Use hpet_get_main_cnt function and do not forget about
 * about pause instruction. */
uint64_t
hpet_cpu_frequency(void) {
    static uint64_t cpu_freq = 0;

    // LAB 5: Your code here

    if (cpu_freq != 0)
    {
        return cpu_freq;
    }

    const uint64_t initial_tsc = read_tsc();

    const uint64_t initial_cnt = hpet_get_main_cnt();
    const uint64_t SCALE_TO_SECONDS = 100ULL;
    const uint64_t delta_cnt = hpetFreq / SCALE_TO_SECONDS;
    const uint64_t final_cnt = initial_cnt + delta_cnt;
    while (hpet_get_main_cnt() < final_cnt) {
        asm volatile("pause");
    }

    const uint64_t final_tsc = read_tsc();

    cpu_freq = (final_tsc - initial_tsc) * SCALE_TO_SECONDS;

    return cpu_freq;
}

uint32_t
pmtimer_get_timeval(void) {
    FADT *fadt = get_fadt();
    return inl(fadt->PMTimerBlock);
}

/* Calculate CPU frequency in Hz with the help with ACPI PowerManagement timer.
 * HINT Use pmtimer_get_timeval function and do not forget that ACPI PM timer
 *      can be 24-bit or 32-bit. */
uint64_t
pmtimer_cpu_frequency(void) {
    static uint64_t cpu_freq = 0;

    // LAB 5: Your code here

    if (cpu_freq != 0)
    {
        return cpu_freq;
    }

    const uint32_t MASK_32_BIT_CNT_MODE = 1U << 8;
    FADT *fadt = get_fadt();

    const bool is_32_bit_cnt = ((fadt->Flags) & MASK_32_BIT_CNT_MODE) != 0;
    const uint32_t mask = is_32_bit_cnt ? 0xFFFFFFFFU : 0x00FFFFFFU;

    const uint64_t initial_tsc = read_tsc();

    const uint32_t initial_timeval = pmtimer_get_timeval();
    const uint32_t SCALE_TO_SECONDS = 100U;
    const uint32_t delta_timeval = PM_FREQ / SCALE_TO_SECONDS;
    while (true) {
        // https://stackoverflow.com/questions/40731543/implementing-enforcing-wraparound-arithmetic-in-c
        const uint32_t current_timeval = pmtimer_get_timeval() & mask;
        const uint32_t elapsed_timeval = (current_timeval - initial_timeval) & mask;
        if (elapsed_timeval >= delta_timeval) {
            break;
        }

        asm volatile("pause");
    }

    const uint64_t final_tsc = read_tsc();

    cpu_freq = (final_tsc - initial_tsc) * SCALE_TO_SECONDS;

    return cpu_freq;
}
