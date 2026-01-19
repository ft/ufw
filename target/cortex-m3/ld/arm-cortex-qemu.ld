/*
 * Copyright (c) 2020-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 *
 * This file can be used to link programs to run on QEMU,  using the
 * lm3s6965evb machine. That's a Cortex-M3 by Texas Instruments. Its
 * data-sheet is available online:
 *
 *   https://www.ti.com/lit/ds/symlink/lm3s6965.pdf
 *
 * The general ARM Cortex-M3 technical reference manual from ARM:
 *
 *   https://static.docs.arm.com/100165/0201/arm_cortexm3_processor_trm_100165_0201_00_en.pdf
 *
 * Another interesting read is “ELF for the ARM architecture”:
 *
 *   https://documentation-service.arm.com/static/5fa189bcb209f547eebd2d92
 *
 * Also, the QEMU user's guide is useful:
 *
 *   https://qemu.readthedocs.io/en/latest/system/index.html
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

ENTRY(_ufw_system_start)

/* Specify the memory areas */
MEMORY
{
  FLASH (xr )  : ORIGIN = 0x00000000, LENGTH = 256K
  RAM   (xrw)  : ORIGIN = 0x20000000, LENGTH =  64K
}

heap_size = 32K;

/* Align regions at 32 bit boundaries */
GENERAL_ALIGNMENT = 4;

REGION_ALIAS("MEMORY_VECTOR_TABLE", FLASH)
REGION_ALIAS("MEMORY_RODATA",       FLASH)
REGION_ALIAS("MEMORY_TEXT",         FLASH)
REGION_ALIAS("MEMORY_ARM_EXTAB",    FLASH)
REGION_ALIAS("MEMORY_ARM_EXIDX",    FLASH)
REGION_ALIAS("MEMORY_DATA",         RAM)
REGION_ALIAS("MEMORY_BSS",          RAM)
REGION_ALIAS("MEMORY_HEAP",         RAM)
REGION_ALIAS("MEMORY_STACK",        RAM)

SECTIONS {
    . = ORIGIN(MEMORY_VECTOR_TABLE);

    .vector_table : {
        . = ALIGN(GENERAL_ALIGNMENT);
        KEEP(*(.vector_table));
        . = ALIGN(GENERAL_ALIGNMENT);
    } >MEMORY_VECTOR_TABLE

    .rodata : {
        . = ALIGN(GENERAL_ALIGNMENT);
        _start_read_only = .;
        *(.rodata)
        *(.rodata*)
        _end_read_only = .;
    } >MEMORY_RODATA

    .text : {
        . = ALIGN(GENERAL_ALIGNMENT);
        _start_text = .;
        *(.text)
        *(.text*)
        _end_text = .;
    } >MEMORY_TEXT

    .ARM.extab : {
            *(.ARM.extab*)
            *(.gnu.linkonce.armextab.*)
    } >MEMORY_ARM_EXTAB

    __exidx_start = .;
    .ARM.exidx : {
            *(.ARM.exidx*)
            *(.gnu.linkonce.armexidx.*)
    } >MEMORY_ARM_EXIDX
    __exidx_end = .;

    __src_data = .;
    .data : AT(__src_data) {
        _start_data = .;
        *(.data)
        *(.data*)
        . = ALIGN(GENERAL_ALIGNMENT);
        _end_data = .;
    } >MEMORY_DATA

    .bss : {
         . = ALIGN(GENERAL_ALIGNMENT);
        _start_bss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(GENERAL_ALIGNMENT);
        _end_bss = .;
    } >MEMORY_BSS

    . = ALIGN(GENERAL_ALIGNMENT);
    .heap : {
        /*
         * In newlib's syscalls.c you'll find this:
         *
         * extern char end asm("end"); // Defined by the linker.
         *
         * So without defining ‘end’ we would get a linker error.
         */
        PROVIDE(end = .);
        PROVIDE(_heap_start = end );
        . += heap_size;
        PROVIDE(_heap_end = .);
    } >MEMORY_HEAP

    .ARM.attributes 0 : {
        *(.ARM.attributes)
    }

    .stack : {
        . = ALIGN(GENERAL_ALIGNMENT);
        _stack_top = .;
        KEEP(*(.stack*))
    } >MEMORY_STACK

    __start_stack = ORIGIN(RAM) + LENGTH(RAM);
    __end_stack = __start_stack - SIZEOF(.stack);
    PROVIDE(__stack = __start_stack);
    ASSERT(__end_stack >= _heap_end, "Heap and stack regions overlap!")
}
_end = .;
