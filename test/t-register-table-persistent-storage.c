/*
 * Copyright (c) 2021-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/test/tap.h>

#define REGISTER_TABLE_WITH_NAMES

#include <ufw/binary-format.h>
#include <ufw/register-table.h>
#include <ufw/register-utilities.h>
#include <ufw/persistent-storage.h>

#include "../src/registers/internal.h"

/* Register table macros */
#define VOLATILE_START            0x2000ul
#define VOLATILE_SIZE             0x308ul

#define PERSISTENT_START          0x3000ul
#define EEPROM_START              0x100ul
#define PERSISTENT_EEPROM_SIZE    ((sizeof(RegisterAtom) * VOLATILE_SIZE) + 2)

#define VOLATILE(x)     (VOLATILE_START + (x))
#define PERSISTENT(x)   (PERSISTENT_START + (x))

#define cmp_code(a, op, b, ...)                 \
    unless (ok(a op b, __VA_ARGS__)) {          \
        pru32(a, b);                            \
    }

static char*
type2string(RegisterType type)
{
    if (type > REG_TYPE_INVALID)
        return "<INVALID-TYPE>";

    char *map[] = {
        register_registertype_to_string(REG_TYPE_INVALID),
        register_registertype_to_string(REG_TYPE_UINT16),
        register_registertype_to_string(REG_TYPE_UINT32),
        register_registertype_to_string(REG_TYPE_UINT64),
        register_registertype_to_string(REG_TYPE_SINT16),
        register_registertype_to_string(REG_TYPE_SINT32),
        register_registertype_to_string(REG_TYPE_SINT64),
        register_registertype_to_string(REG_TYPE_FLOAT32)
    };
    return map[type];
}
/* Persistent storage infrastructure */

PersistentStorage storage;

#define BUFFER_SIZE (2048)
unsigned char persist_mem[BUFFER_SIZE];

static size_t
buffer_read(void *buf, uint32_t addr, size_t n)
{
    if ((addr + n) > BUFFER_SIZE)
        return 0;

    memcpy(buf, persist_mem + addr, n);
    return n;
}

static size_t
buffer_write(uint32_t addr, const void *buf, size_t n)
{
    if ((addr + n) > BUFFER_SIZE)
        return 0;

    memcpy(persist_mem + addr, buf, n);
    return n;
}

static RegisterAccess
persistent_to_register(const RegisterArea *a, RegisterOffset o,
                       PersistentAccess access)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    switch (access) {
    case PERSISTENT_ACCESS_SUCCESS:
        return rv;
    case PERSISTENT_ACCESS_INVALID_DATA:
        rv.code = REG_ACCESS_INVALID;
        break;
    case PERSISTENT_ACCESS_IO_ERROR:
        rv.code = REG_ACCESS_IO_ERROR;
        break;
    case PERSISTENT_ACCESS_ADDRESS_OUT_OF_RANGE: /* FALLTHROUGH */
    default:
        rv.code = REG_ACCESS_RANGE;
        break;
    }
    rv.address = a->base + o;
    return rv;
}

static RegisterAccess
persist_read(const RegisterArea *a, RegisterAtom *dest, RegisterOffset o,
         RegisterOffset n)
{
    size_t nbytes = sizeof(RegisterAtom) * n;
    return persistent_to_register(
        a, o, persistent_fetch_part(dest, &storage, o, nbytes));
}

static RegisterAccess
persist_write(RegisterArea *a, const RegisterAtom *dest, RegisterOffset o,
          RegisterOffset n)
{
    size_t nbytes =  sizeof(RegisterAtom) * n;
    return persistent_to_register(
        a, o, persistent_store_part(&storage, dest, o, nbytes));
}

/* Register table infrastructure */
#define REG_SETUP(ID)        \
    ID##1,                   \
    ID##2,                   \
    ID##3,                   \
    ID##4,                   \
    ID##5,                   \
    ID##6,                   \
    ID##7,                   \
    ID##8,                   \
    ID##9,                   \
    ID##10,                  \
    ID##11,                  \
    ID##12,                  \
    ID##13,                  \
    ID##14,                  \
    ID##15,                  \
    ID##16,                  \
    ID##17,                  \
    ID##18,                  \
    ID##19,                  \
    ID##20,                  \
    ID##21,                  \
    ID##22,                  \
    ID##23,                  \
    ID##24,                  \
    ID##25,                  \
    ID##26,                  \
    ID##27,                  \
    ID##28,                  \
    ID##29,                  \
    ID##30,                  \
    ID##31

#define REG_ENTRY_SETUP(ID,ADDRMOD)             \
    REG_U16(ID##1,  ADDRMOD(0x00ul),   0x00ul), \
    REG_U32(ID##2,  ADDRMOD(0x01ul),   0x01ul), \
    REG_U16(ID##3,  ADDRMOD(0x03ul),   0x03ul), \
    REG_U16(ID##4,  ADDRMOD(0x04ul),   0x04ul), \
    REG_U16(ID##5,  ADDRMOD(0x05ul),   0x05ul), \
    REG_U32(ID##6,  ADDRMOD(0x06ul),   0x06ul), \
    REG_U32(ID##7,  ADDRMOD(0x08ul),   0x08ul), \
    REG_U32(ID##8,  ADDRMOD(0x0aul),   0x0aul), \
    REG_U16(ID##9,  ADDRMOD(0x0cul),   0x0cul), \
    REG_U16(ID##10, ADDRMOD(0x0dul),   0x0dul), \
    REG_U16(ID##11, ADDRMOD(0x0eul),   0x0eul), \
    REG_U16(ID##12, ADDRMOD(0x0ful),   0x0ful), \
    REG_U32(ID##13, ADDRMOD(0x10ul),   0x10ul), \
    REG_U16(ID##14, ADDRMOD(0x12ul),   0x12ul), \
    REG_U32(ID##15, ADDRMOD(0x14ul),   0x14ul), \
    REG_U16(ID##16, ADDRMOD(0x16ul),   0x16ul), \
    REG_U16(ID##17, ADDRMOD(0x18ul),   0x18ul), \
    REG_U16(ID##18, ADDRMOD(0x19ul),   0x19ul), \
    REG_U32(ID##19, ADDRMOD(0x100ul), 0x100ul), \
    REG_U16(ID##20, ADDRMOD(0x102ul), 0x102ul), \
    REG_U32(ID##21, ADDRMOD(0x103ul), 0x103ul), \
    REG_U16(ID##22, ADDRMOD(0x105ul), 0x105ul), \
    REG_U32(ID##23, ADDRMOD(0x106ul), 0x106ul), \
    REG_U32(ID##24, ADDRMOD(0x108ul), 0x108ul), \
    REG_U32(ID##25, ADDRMOD(0x200ul), 0x200ul), \
    REG_U32(ID##26, ADDRMOD(0x202ul), 0x202ul), \
    REG_U32(ID##27, ADDRMOD(0x204ul), 0x204ul), \
    REG_U32(ID##28, ADDRMOD(0x300ul), 0x300ul), \
    REG_U16(ID##29, ADDRMOD(0x302ul), 0x302ul), \
    REG_U16(ID##30, ADDRMOD(0x303ul), 0x303ul), \
    REG_U64(ID##31, ADDRMOD(0x304ul), 0x304ul)

typedef enum SensorRegisterArea {
    REG_AREA_VOLATILE,
    REG_AREA_PERSISTENT
} SensorRegisterArea;

typedef enum SensorRegister {
    REG_SETUP(VOLATILE_),
    REG_SETUP(PERSISTENT_)
} SensorRegister;

RegisterTable registers = {
    .area = (RegisterArea[]) {
        [REG_AREA_VOLATILE] = MEMORY_AREA(VOLATILE_START, VOLATILE_SIZE),
        [REG_AREA_PERSISTENT] = MAKE_CUSTOM_AREA(persist_read, persist_write,
                                                 PERSISTENT_START,
                                                 PERSISTENT_EEPROM_SIZE,
                                                 REG_AF_RW
                                                 | REG_AF_SKIP_DEFAULTS),
        REGISTER_AREA_END
    },
    .entry = (RegisterEntry[]) {
        REG_ENTRY_SETUP(VOLATILE_, VOLATILE),
        REG_ENTRY_SETUP(PERSISTENT_, PERSISTENT),
        REGISTER_ENTRY_END
    }
};

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(10);
    memset(persist_mem, 0xff, BUFFER_SIZE);

    const bool showtable = (argc == 2) && (strcmp(argv[1], "show-table") == 0);
    const RegisterInit bootsuccess = register_init(&registers);

    cmp_code(registers.entries, == , 62u,
             "Register table has the correct number of entries");
    cmp_code(registers.areas, == , 2u,
             "Register table has the correct number of areas");

    if (bootsuccess.code != REG_INIT_SUCCESS  || showtable) {
        register_table_print(stdout, "# ", &registers);
        printf("#\n");
        register_init_print(stdout, "# ", bootsuccess);
        if (showtable)
            return EXIT_SUCCESS;
    }

    persistent_init(&storage, PERSISTENT_EEPROM_SIZE, buffer_read, buffer_write);
    persistent_place(&storage, EEPROM_START);

    char prbuff[BUFFER_SIZE] = {0,};
    memset(prbuff, 0xcd, BUFFER_SIZE);

    size_t rbytes = VOLATILE_SIZE;
    size_t pbytes = (sizeof(RegisterAtom) * rbytes);
    RegisterAccess res;

    /* This block checks whether persistent memory is initialized with default
     * values. It is supposed not to be initialzed due to REG_AF_SKIP_DEFAULTS
     * flag. */
    {
        res = register_block_read(&registers, PERSISTENT_START, rbytes, (void*) prbuff);
        ok(res.code == REG_ACCESS_SUCCESS,
           "Block read %d bytes from persistent area, 0x%x", pbytes, PERSISTENT_START);

        cmp_mem(persist_mem, prbuff, pbytes,
                "Read data from volatile and persistent match!");
    }

    /* This block tests individual register access in the persistent memory area */
    {
        RegisterValue v = (RegisterValue) {.type = REG_TYPE_UINT16, .value.u16 = 0x100};
        RegisterValue rv;

        register_set(&registers, PERSISTENT_10, v);
        register_get(&registers, PERSISTENT_10, &rv);

        cmp_code(rv.type, ==, REG_TYPE_UINT16,
                  "Gotten value is type u16", type2string(rv.type));
        cmp_code(rv.value.u16, ==, v.value.u16,
                  "Gotten value is correct value");
    }

    /* Test register block access in the persistent memory area */
    res = register_mcopy(&registers, REG_AREA_PERSISTENT, REG_AREA_VOLATILE);
    unless (ok(res.code == REG_ACCESS_SUCCESS,
               "register_mcopy from VOLATILE area to PERSISTENT area successful"))
    {
        pru16(res.code, REG_ACCESS_SUCCESS);
    }

    /* Verify the contents in the volatile and persistent blocks*/
    {
        char vrbuff[BUFFER_SIZE] = {0,};
        memset(vrbuff, 0xab, BUFFER_SIZE);

        res = register_block_read(&registers, PERSISTENT_START, rbytes, (void*) prbuff);
        ok(res.code == REG_ACCESS_SUCCESS,
           "Block read %d bytes from persistent area, 0x%x", pbytes, PERSISTENT_START);

        res = register_block_read(&registers, VOLATILE_START, rbytes, (void*) vrbuff);
        ok(res.code == REG_ACCESS_SUCCESS,
           "Block read %d bytes from volatile area 0x%x", pbytes, VOLATILE_START);

        cmp_mem(vrbuff, prbuff, pbytes,
                "Read data from volatile and persistent match!");
    }
    return EXIT_SUCCESS;
}
