/*
 * Copyright (c) 2020-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file startup.c
 * @brief ARM Cortex M3 start-up code
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <ufw/compiler.h>

/* Prototype for user-supplied code; definition is *not* part of this file. */
int main(void);

/* This is newlib's callback that enables stdio via semihosting. This startup
 * file enables this by default before running the main sub-routine. */
void initialise_monitor_handles(void);

/*
 * Addresses defined in linker script
 *
 * As a convention, the script uses double underscores to indicate locations in
 * ROM, while single underscored names indicate RAM locations. Most importantly
 * here, __src_data marks the location where the .data section is stored in the
 * program's ROM image. The startup routine will copy from here into the .data
 * section's RAM location.
 */
extern uint32_t __src_data;
extern uint32_t _stack_top;
extern uint32_t _start_data;
extern uint32_t _end_data;
extern uint32_t _start_bss;
extern uint32_t _end_bss;

/*
 * Vector table setup
 */

typedef void(*_ufw_isr_handler)(void);

union _ufw_vector_entry {
    _ufw_isr_handler isr;
    uint32_t *address;
};

/* cortex-m3 core isr prototypes */
void _ufw_system_start(void);
void _ufw_isr_fallback(void);
void _ufw_isr_nmi(void)             WEAK_ALIAS(_ufw_isr_fallback);
void _ufw_isr_hard_fault(void)      WEAK_ALIAS(_ufw_isr_fallback);
void _ufw_isr_memory_manage(void)   WEAK_ALIAS(_ufw_isr_fallback);
void _ufw_isr_bus_fault(void)       WEAK_ALIAS(_ufw_isr_fallback);
void _ufw_isr_usage_fault(void)     WEAK_ALIAS(_ufw_isr_fallback);
void _ufw_isr_supervisor_call(void) WEAK_ALIAS(_ufw_isr_fallback);
void _ufw_isr_debug(void)           WEAK_ALIAS(_ufw_isr_fallback);
void _ufw_isr_service_request(void) WEAK_ALIAS(_ufw_isr_fallback);
void _ufw_isr_system_tick(void)     WEAK_ALIAS(_ufw_isr_fallback);

/* Some entries in the cortey-m3 vector table are reserved. */
#define __RESERVED_ENTRY__ NULL

/* Stack top and vector handler table */
union _ufw_vector_entry vector_table[] SECTION(".vector_table") = {
    /* Initial value of stack-pointer: */
    [ 0] = { .address = &_stack_top          },
    /* Main entry-point that gets run as soon as possible after system
     * start. It initialises all required memory and runs main(). */
    [ 1] = { .isr = _ufw_system_start        },
    [ 2] = { .isr = _ufw_isr_nmi             },
    [ 3] = { .isr = _ufw_isr_hard_fault      },
    [ 4] = { .isr = _ufw_isr_memory_manage   },
    [ 5] = { .isr = _ufw_isr_bus_fault       },
    [ 6] = { .isr = _ufw_isr_usage_fault     },
    [ 7] = { .isr = __RESERVED_ENTRY__       },
    [ 8] = { .isr = __RESERVED_ENTRY__       },
    [ 9] = { .isr = __RESERVED_ENTRY__       },
    [10] = { .isr = __RESERVED_ENTRY__       },
    [11] = { .isr = _ufw_isr_supervisor_call },
    [12] = { .isr = _ufw_isr_debug           },
    [13] = { .isr = __RESERVED_ENTRY__       },
    [14] = { .isr = _ufw_isr_service_request },
    [15] = { .isr = _ufw_isr_system_tick     }
};

/*
 * System control-block definition
 */

#define _io_ volatile

struct _cortex_m3_system_ctrl {
    _io_ uint32_t cpuid; /* CPUID Base Register */
    _io_ uint32_t icsr;  /* Interrupt Control and State Register */
    _io_ uint32_t vtor;  /* Vector Table Offset Register */
    _io_ uint32_t aircr; /* Application Interrupt and Reset Control Register */
    _io_ uint32_t scr;   /* System Control Register */
    _io_ uint32_t ccr;   /* Configuration and Control Register */
    _io_ uint32_t shpr1; /* System Handler Priority Register 1 */
    _io_ uint32_t shpr2; /* System Handler Priority Register 2 */
    _io_ uint32_t shpr3; /* System Handler Priority Register 3 */
    _io_ uint32_t shcrs; /* System Handler Control and State Register */
    _io_ uint32_t cfsr;  /* Configurable Fault Status Register */
    _io_ uint8_t  mmsr;  /* Memory Management Fault Status Register */
    _io_ uint8_t  bfsr;  /* Bus Fault Status Register */
    _io_ uint16_t ufsr;  /* UsageFault Status Register */
    _io_ uint32_t hfsr;  /* HardFault Status Register */
    _io_ uint32_t mmar;  /* Memory Management Fault Address Register */
    _io_ uint32_t bfar;  /* Bus Fault Address Register */
    _io_ uint32_t afsr;  /* Auxiliary Fault Status Register */
};

struct _cortex_m3_system_ctrl *_ufw_system_ctrl = (void*)0xe000ed00;

void _ufw_system_reset(void);

void
_ufw_system_reset(void)
{
    _ufw_system_ctrl->aircr = 0x05FA0004;
}

/* Default interrupt service routine */

void
_ufw_isr_fallback(void)
{
    for (;;) {
        asm(" nop");
    }
}

/* Primary system entry point. */

void
_ufw_system_start(void) {
    const size_t databytes = (&_end_data - &_start_data) * sizeof(uint32_t);
    const size_t bssbytes  = (&_end_bss  - &_start_bss)  * sizeof(uint32_t);

    memcpy(&_start_data, &__src_data, databytes);
    memset(&_start_bss, 0, bssbytes);

    /*
     * Initialise semihosting for easy program output from qemu.
     *
     * Soft-reset system after main is done. If you use -no-reboot with qemu,
     * you can effectively use qemu to run cortex-m3 binaries from the command
     * line.
     */
    initialise_monitor_handles();
    main();
    _ufw_system_reset();
}
