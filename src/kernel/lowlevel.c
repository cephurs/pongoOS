// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// 
//
//  Copyright (c) 2019-2020 checkra1n team
//  This file is part of pongoOS.
//
#define LL_KTRW_INTERNAL 1
#include <pongo.h>
uint64_t gInterruptBase;
uint64_t gPMGRBase;
uint64_t gWDTBase;

__asm__(
    ".globl get_el\n"
    ".globl rebase_pc\n"
    ".globl set_vbar_el3\n"
    ".globl set_vbar_el1\n"
    ".globl get_el\n"
    ".globl get_ticks\n"
    ".globl invalidate_icache\n"
    ".globl _enable_interrupts\n"
    ".globl _disable_interrupts\n"
    "get_el:\n"
    "    mrs x0, currentel\n"
    "    lsr x0, x0, 2\n"
    "    ret\n"
    "rebase_pc:\n"
    "    add sp, sp, x0\n"
    "    add x29, x29, x0\n"
    "    add x30, x30, x0\n"
    "    ret\n"

    "set_vbar_el3:\n"
    "    msr vbar_el3, x0\n"
    "    isb\n"
    "    ret\n"

    "set_vbar_el1:\n"
    "    msr vbar_el1, x0\n"
    "    isb\n"
    "    ret\n"

    "_enable_interrupts:\n"
    "    msr daifclr,#0xf\n"
    "    isb\n"
    "    ret\n"
    "_disable_interrupts:\n"
    "    msr daifset,#0xf\n"
    "    isb\n"
    "    ret\n"

    "get_mpidr:\n"
    "    mrs x0, MPIDR_EL1\n"
    "    ret\n"
    "get_migsts:\n"
    "    mrs x0, S3_4_c15_c0_4\n"
    "    ret\n"
    "set_migsts:\n"
    "    msr S3_4_c15_c0_4, x0\n"
    "    ret\n"
    "get_mmfr0:\n"
    "    mrs x0, id_aa64mmfr0_el1\n"
    "    ret\n"
    "invalidate_icache:\n"
    "    dsb ish\n"
    "    ic iallu\n"
    "    dsb ish\n"
    "    isb\n"
    "    ret\n"

    "enable_mmu_el1:\n"
    "    dsb sy\n"
    "    msr mair_el1, x2\n"
    "    msr tcr_el1, x1\n"
    "    msr ttbr0_el1, x0\n"
    "    isb sy\n"
    "    ic iallu\n"
    "    isb sy\n"
    "    mrs x3, sctlr_el1\n"
    "    orr x3, x3, #1\n"
    "    orr x3, x3, #4\n"
    "    and x3, x3, #(~2)\n"
    "    msr sctlr_el1, x3\n"
    "    ic iallu\n"
    "    dsb sy\n"
    "    isb sy\n"
    "    ret\n"

    "disable_mmu_el1:\n"
    "    dsb sy\n"
    "    isb sy\n"
    "    mrs x3, sctlr_el1\n"
    "    and x3, x3, #(~1)\n"
    "    and x3, x3, #(~4)\n"
    "    msr sctlr_el1, x3\n"
    "    tlbi vmalle1\n"
    "    ic iallu\n"
    "    dsb sy\n"
    "    isb sy\n"
    "    ret\n"

    "enable_mmu_el3:\n"
    "    dsb sy\n"
    "    msr mair_el3, x2\n"
    "    msr tcr_el3, x1\n"
    "    msr ttbr0_el3, x0\n"
    "    isb sy\n"
    "    ic iallu\n"
    "    isb sy\n"
    "    mrs x3, sctlr_el3\n"
    "    orr x3, x3, #1\n"
    "    orr x3, x3, #4\n"
    "    and x3, x3, #(~2)\n"
    "    msr sctlr_el3, x3\n"
    "    isb sy\n"
    "    mrs x3, scr_el3\n"
    "    orr x3, x3, #6\n"
    "    msr scr_el3, x3\n"
    "    ret\n"

    "disable_mmu_el3:\n"
    "    dsb sy\n"
    "    isb sy\n"
    "    mrs x3, sctlr_el3\n"
    "    and x3, x3, #(~1)\n"
    "    and x3, x3, #(~3)\n"
    "    msr sctlr_el3, x3\n"
    "    tlbi alle3\n"
    "    ic iallu\n"
    "    dsb sy\n"
    "    isb sy\n"
    "    ret\n"

    "get_ticks:\n"
    "   isb sy\n"
    "   mrs x0, cntpct_el0\n"
    "   ret\n"
    );

uint64_t dis_int_count = 1;
void _enable_interrupts();
void enable_interrupts() {
    if (!dis_int_count) panic("irq over-enable");
    dis_int_count--;
    if (!dis_int_count) {
        _enable_interrupts();
    }
}
void _disable_interrupts();
void disable_interrupts() {
    _disable_interrupts();
    dis_int_count++;
    if (!dis_int_count) panic("irq over-disable");
}

OBFUSCATE_C_FUNC(static _Bool is_16k(void))
{
    return ((get_mmfr0() >> 20) & 0xf) == 0x1;
}
volatile char is_in_exception;
void print_state(uint64_t* state) {
    for (int i=0; i<31; i++) {
        iprintf("X%d: 0x%016lx\n", i, state[i]);
    }
    iprintf("ESR_EL1: 0x%016lx\n", state[0xf8/8]);
    iprintf("ELR_EL1: 0x%016lx\n", state[0x100/8]);
    iprintf("FAR_EL1: 0x%016lx\n", state[0x108/8]);
}
int sync_exc(uint64_t* state) {
    dis_int_count = 1;
    if (!is_in_exception && task_current()->flags & TASK_CAN_CRASH) {
        is_in_exception = 1;

        iprintf("pongoOS: killing task |%s| due to crash\n", task_current()->name[0] ? task_current()->name : "<unknown>");
        print_state(state);

        task_unlink(task_current());
        task_current()->flags &= ~TASK_CAN_CRASH;
        task_current()->flags |= TASK_HAS_EXITED;
        task_current()->flags |= TASK_HAS_CRASHED;
        is_in_exception = 0;
        task_yield_asserted();
        return 0;
    }
    print_state(state);
    panic("caught sync exception");
    return 0;
}
uint32_t interrupt_vector() {
    return (*(volatile uint32_t *)(gInterruptBase + 0x2004));
}
uint64_t interruptCount = 0, fiqCount = 0;
uint32_t do_preempt = 1;
void disable_preemption() {
    disable_interrupts();
    do_preempt++;
    enable_interrupts();
}
void enable_preemption() {
    disable_interrupts();
    do_preempt--;
    enable_interrupts();
}
int irq_exc() {
    timer_disable();
    dis_int_count = 1;
    is_in_exception = 1;
    interruptCount++;
    if (task_current()->irq_ret) {
        panic("nested IRQs are not supported at this time");
    }
    uint32_t intr = interrupt_vector();
    while (intr) {
        task_irq_dispatch(intr);
        intr = interrupt_vector();
    }
    if (dis_int_count != 1) panic("IRQ handler left interrupts disabled...");
    is_in_exception = 0;
    dis_int_count = 0;
    timer_enable();
    return 0;
}
int serror_exc(uint64_t* state) {
    is_in_exception = 1;
    dis_int_count = 1;
    print_state(state);
    panic("caught serror exception");
    is_in_exception = 0;
    return 0;
}
int _fiq_exc() {
    is_in_exception = 1;
    fiqCount++;
    dis_int_count = 1;
    wdt_enable();
    int ret_val = pongo_fiq_handler();
    if (dis_int_count != 1) panic("FIQ handler left interrupts disabled...");
    dis_int_count = 0;
    is_in_exception = 0;
    return ret_val;
}
extern uint64_t preemption_counter;
int fiq_exc() {
    int fiq_r = _fiq_exc();
    if (fiq_r) {
        preemption_counter++;
    }
    return fiq_r;
}
void spin(uint32_t usec)
{
    disable_interrupts();
    uint64_t eta_now = get_ticks();
    uint64_t eta_wen = eta_now + (24*usec);
    while (1) {
        asm volatile("isb");
        uint32_t curtime = get_ticks();
        if (eta_now > curtime)
            break;
        if (curtime > eta_wen)
            break;
    }
    enable_interrupts();
}
void usleep(uint32_t usec)
{
    disable_interrupts();
    uint64_t eta_wen = get_ticks() + (24*usec);
    uint64_t preempt_after = get_ticks() + 2400;
    task_current()->wait_until = eta_wen;
    enable_interrupts();
    while (1) {
        uint32_t curtime = get_ticks();
        if (curtime > eta_wen)
            break;
        if (curtime > preempt_after)
            task_yield();
    }
}
void sleep(uint32_t sec)
{
    usleep(sec*1000*1000);
}
int gAICVersion = -1;
int gAICStyle = -1;

static void interrupt_or_config(uint32_t bits) {
    *(volatile uint32_t*)(gInterruptBase + 0x10) |= bits;
}
static void interrupt_and_config(uint32_t bits) {
    *(volatile uint32_t*)(gInterruptBase + 0x10) &= bits;
}
uint32_t interrupt_masking_base = 0;
void unmask_interrupt(uint32_t reg) {
    (*(volatile uint32_t *)(gInterruptBase + 0x4180 + ((reg >> 5) * 4))) = (1 << ((reg) & 0x1F));

}
void mask_interrupt(uint32_t reg) {
    (*(volatile uint32_t *)(gInterruptBase + 0x4100 + ((reg >> 3) * 4))) = (1 << ((reg) & 0x1F));
}
void wdt_reset() {
    if (!gWDTBase) {
        panic("wdt is not set up but was asked to reset, spinning here");
        while (1) {}
    }
    *(volatile uint32_t*)(gWDTBase + 0xc) = 0;
    *(volatile uint32_t*)(gWDTBase + 0x4) = 1;
    *(volatile uint32_t*)(gWDTBase + 0x0) = 0x80000000;
    *(volatile uint32_t*)(gWDTBase + 0xc) = 4;
    *(volatile uint32_t*)(gWDTBase + 0x0) = 0;
    panic("wdt reset");
}
void wdt_enable() {
    return;
    if (!gWDTBase) return;
    *(volatile uint32_t*)(gWDTBase + 0xc) = 0;
    *(volatile uint32_t*)(gWDTBase + 0x4) = 5 * 24000000; // fire watchdog reset every 5 seconds
    *(volatile uint32_t*)(gWDTBase + 0x0) = 0x80000000;
    *(volatile uint32_t*)(gWDTBase + 0xc) = 4;
    *(volatile uint32_t*)(gWDTBase + 0x0) = 0;
}
void wdt_disable() {
    return;
    if (!gWDTBase) return;
    *(volatile uint32_t*)(gWDTBase + 0xc) = 0;
    *(volatile uint32_t*)(gWDTBase + 0x4) = 0xffffffff;
    *(volatile uint32_t*)(gWDTBase + 0x0) = 0x80000000;
    *(volatile uint32_t*)(gWDTBase + 0xc) = 4;
    *(volatile uint32_t*)(gWDTBase + 0x0) = 0;
}

void pmgr_init() {
    gPMGRBase = dt_get_u32_prop("pmgr", "reg");
    gPMGRBase += gIOBase;
    gWDTBase = dt_get_u32_prop("wdt", "reg");
    gWDTBase += gIOBase;
    command_register("reset", "resets the device", wdt_reset);
    command_register("crash", "branches to an invalid address", (void*)0x41414141);
}
void interrupt_init() {
    gInterruptBase = dt_get_u32_prop("aic", "reg");
    gInterruptBase += gIOBase;

    gAICVersion = dt_get_u32_prop("aic", "aic-version");
    iprintf("initializing AIC %d\n", gAICVersion);

    interrupt_or_config(0xE0000000);
    interrupt_or_config(1); // enable interrupt
}
void interrupt_teardown() {
    wdt_disable();
    task_irq_teardown();
}

void clock_gate(uint64_t addr, char val)
{
    if (val) {
        *(volatile uint32_t*)(addr) |= 0xF;
    } else {
        *(volatile uint32_t*)(addr) &= ~0xF;
    }

    while (1) {
        volatile uint32_t a = *(volatile uint32_t*)(addr);
        volatile uint32_t b = *(volatile uint32_t*)(addr);
        a ^= b >> 4;
        a &= 0xf;
        if (!a) break;
    }
}

void
cache_invalidate(void *address, size_t size) {
    uint64_t cache_line_size = 64;
    uint64_t start = ((uintptr_t) address) & ~(cache_line_size - 1);
    uint64_t end = ((uintptr_t) address + size + cache_line_size - 1) & ~(cache_line_size - 1);
    asm volatile("isb");
    asm volatile("dsb sy");
    for (uint64_t addr = start; addr < end; addr += cache_line_size) {
        asm volatile("dc ivac, %0" : : "r"(addr));
    }
    asm volatile("dsb sy");
    asm volatile("isb");
}

void
cache_clean_and_invalidate(void *address, size_t size) {
    uint64_t cache_line_size = 64;
    uint64_t start = ((uintptr_t) address) & ~(cache_line_size - 1);
    uint64_t end = ((uintptr_t) address + size + cache_line_size - 1) & ~(cache_line_size - 1);
    asm volatile("isb");
    asm volatile("dsb sy");
    for (uint64_t addr = start; addr < end; addr += cache_line_size) {
        asm volatile("dc civac, %0" : : "r"(addr));
    }
    asm volatile("dsb sy");
    asm volatile("isb");
}


void
cache_clean(void *address, size_t size) { // invalidates too, because Apple
    uint64_t cache_line_size = 64;
    uint64_t start = ((uintptr_t) address) & ~(cache_line_size - 1);
    uint64_t end = ((uintptr_t) address + size + cache_line_size - 1) & ~(cache_line_size - 1);
    asm volatile("isb");
    asm volatile("dsb sy");
    for (uint64_t addr = start; addr < end; addr += cache_line_size) {
        asm volatile("dc civac, %0" : : "r"(addr));
    }
    asm volatile("dsb sy");
    asm volatile("isb");
}

uint64_t vatophys(uint64_t kvaddr) {
    uint64_t par_el1;
    disable_interrupts();
    if (get_el() == 1) {
        asm volatile("at s1e1r, %0" : : "r"(kvaddr));
        asm volatile("isb");
        asm volatile("mrs %0, PAR_EL1" : "=r"(par_el1));
    } else {
        asm volatile("at S1E3R, %0" : : "r"(kvaddr));
        asm volatile("isb");
        asm volatile("mrs %0, PAR_EL1" : "=r"(par_el1));
    }
    enable_interrupts();
    if (par_el1 & 0x1) {
        return -1;
    }
    par_el1 &= 0xFFFFFFFFFFFF;
    if (is_16k()) {
        return (kvaddr & 0x3FFF) | (par_el1 & (~0x3FFF));
    } else {
        return (kvaddr & 0xFFF) | (par_el1 & (~0xFFF));
    }
}

uint64_t ram_phys_off;
uint64_t ram_phys_size;

uint64_t tt_bits, tg0, t0sz;
uint64_t ttb_alloc_base;
volatile uint64_t *ttbr0;

volatile uint64_t* ttb_alloc(void)
{
    uint64_t pgsz = 1ULL << (tt_bits + 3ULL);
    ttb_alloc_base -= pgsz;
    volatile uint64_t* rv = (volatile uint64_t*) ttb_alloc_base;
    for(size_t i = 0; i < (pgsz / 8); i++)
    {
        rv[i] = 0;
    }
    return rv;
}

void map_range(uint64_t va, uint64_t pa, uint64_t size, uint64_t sh, uint64_t attridx, bool overwrite)
{
    // NOTE: Blind assumption that all TT levels support block mappings.
    // Currently we configure TCR that way, we just need to ensure that we will continue to do so.

    uint64_t pgsz = 1ULL << (tt_bits + 3ULL);
    if((va & (pgsz - 1ULL)) || (pa & (pgsz - 1ULL)) || (size & (pgsz - 1ULL)) || size < pgsz || (va + size < va) || (pa + size < pa))
    {
        iprintf("map_range(0x%lx, 0x%lx, 0x%lx, ...)\n", va, pa, size);
        panic("map_range: called with bad arguments");
    }

    union
    {
        struct
        {
            uint64_t valid :  1,
                     table :  1,
                     attr  :  3,
                     ns    :  1,
                     ap    :  2,
                     sh    :  2,
                     af    :  1,
                     nG    :  1,
                     oa    : 36,
                     res00 :  3,
                     dbm   :  1,
                     cont  :  1,
                     pxn   :  1,
                     uxn   :  1,
                     ign0  :  4,
                     pbha  :  4,
                     ign1  :  1;
        };
        struct
        {
            uint64_t res01  : 12,
                     oahigh :  4,
                     nT     :  1,
                     res02  : 42,
                     pxntab :  1,
                     uxntab :  1,
                     aptab  :  2,
                     nstab  :  1;
        };
        uint64_t u64;
    } tte;

    volatile uint64_t *tt = ttbr0;
    uint64_t bits = 64ULL - t0sz;
    if((bits - 3) % tt_bits != 0)
    {
        bits += tt_bits - ((bits - 3) % tt_bits);
    }
    while(true)
    {
        uint64_t blksz = 1ULL << (bits - tt_bits),
                 lo = va & ~(blksz - 1ULL),
                 hi = (va + size + (blksz - 1ULL)) & ~(blksz - 1ULL);

        if(size < blksz && hi - lo == blksz) // Sub-block, but fits into single TT
        {
            uint64_t idx = (va >> (bits - tt_bits)) & ((1ULL << tt_bits) - 1ULL);
            tte.u64 = tt[idx];
            if(tte.valid && tte.table)
            {
                tt = (volatile uint64_t*)((uint64_t)tte.oa << 12);
            }
            else if(!tte.valid || overwrite)
            {
                volatile uint64_t *newtt = ttb_alloc();
                tte.u64 = 0;
                tte.valid = 1;
                tte.table = 1;
                tte.oa = (uint64_t)newtt >> 12;
                tt[idx] = tte.u64;
                tt = newtt;
            }
            else
            {
                panic("map_range: trying to map table over existing entry");
            }
            bits -= tt_bits;
            continue;
        }

        while(lo < hi)
        {
            uint64_t sz = blksz;
            if(lo < va)
            {
                sz -= va - lo;
            }
            if(sz > size)
            {
                sz = size;
            }
            if(sz < blksz || (pa & (blksz - 1ULL))) // Need to traverse anew
            {
                map_range(va, pa, sz, sh, attridx, overwrite);
            }
            else
            {
                uint64_t idx = (va >> (bits - tt_bits)) & ((1ULL << tt_bits) - 1);
                tte.u64 = tt[idx];
                if(tte.valid && !overwrite)
                {
                    panic("map_range: trying to map block over existing entry");
                }
                tte.u64 = 0;
                tte.valid = 1;
                tte.table = blksz == pgsz ? 1 : 0; // L3
                tte.attr = attridx;
                tte.sh = sh;
                tte.af = 1;
                tte.oa = pa >> 12;
                tt[idx] = tte.u64;
            }
            lo += blksz;
            va += sz;
            pa += sz;
            size -= sz;
        }
        break;
    }
}

OBFUSCATE_C_FUNC(void map_full_ram(uint64_t phys_off, uint64_t phys_size)) {
    // Round up to make sure the framebuffer is in range
    uint64_t pgsz = 1ULL << (tt_bits + 3);
    phys_size = (phys_size + pgsz - 1) & ~(pgsz - 1);

    map_range(kCacheableView + phys_off, 0x800000000 + phys_off, phys_size, 3, 1, true);
    map_range(0x800000000ULL + phys_off, 0x800000000 + phys_off, phys_size, 3, 0, true);
    ram_phys_off = kCacheableView + phys_off;
    ram_phys_size = phys_size;

    asm volatile("dsb sy");
    if (get_el() == 1) {
         asm volatile("tlbi vmalle1\n");
    } else {
         asm volatile("tlbi alle3\n");
    }
    asm volatile("dsb sy");
}

OBFUSCATE_C_FUNC(void lowlevel_setup(uint64_t phys_off, uint64_t phys_size))
{
    if (is_16k()) {
        tt_bits = 11;
        tg0 = 0b10;
        t0sz = 28;
    } else {
        tt_bits = 9;
        tg0 = 0b00;
        t0sz = 25;
    }
    uint64_t pgsz = 1ULL << (tt_bits + 3);

    ttb_alloc_base = MAGIC_BASE - 0x4000;

    ttbr0 = ttb_alloc();
    map_range(0x200000000, 0x200000000, 0x100000000, 3, 0, false);
    phys_off += (pgsz-1);
    phys_off &= ~(pgsz-1);
    map_range(kCacheableView + phys_off, 0x800000000 + phys_off, phys_size, 3, 1, false);
    map_range(0x800000000ULL + phys_off, 0x800000000 + phys_off, phys_size, 3, 0, false);

    ram_phys_off = kCacheableView + phys_off;
    ram_phys_size = phys_size;

    if (get_el() == 1) {
        set_vbar_el1((uint64_t)&exception_vector);
        enable_mmu_el1((uint64_t)ttbr0, 0x130802a00 | (tg0 << 14) | t0sz, 0xbb04, 5);
    } else {
        set_vbar_el3((uint64_t)&exception_vector);
        enable_mmu_el3((uint64_t)ttbr0, 0x12a00 | (tg0 << 14) | t0sz, 0xbb04);
    }
}

OBFUSCATE_C_FUNC(void lowlevel_cleanup(void))
{
    cache_clean_and_invalidate((void*)ram_phys_off, ram_phys_size);
    if (get_el() == 1) {
        disable_mmu_el1();
    } else {
        disable_mmu_el3();
    }
}
