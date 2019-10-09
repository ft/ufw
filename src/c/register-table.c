/*
 * Copyright (c) 2019 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <c/register-table.h>

/*
 * Internal API prototypes
 */

/* Ser/Des */
static bool rds_invalid_ser(const RegisterValue, RegisterAtom*);
static bool rds_invalid_des(const RegisterAtom*, RegisterValue*);
static bool rds_u16_ser(const RegisterValue, RegisterAtom*);
static bool rds_u16_des(const RegisterAtom*, RegisterValue*);
static bool rds_u32_ser(const RegisterValue, RegisterAtom*);
static bool rds_u32_des(const RegisterAtom*, RegisterValue*);
static bool rds_u64_ser(const RegisterValue, RegisterAtom*);
static bool rds_u64_des(const RegisterAtom*, RegisterValue*);
static bool rds_s16_ser(const RegisterValue, RegisterAtom*);
static bool rds_s16_des(const RegisterAtom*, RegisterValue*);
static bool rds_s32_ser(const RegisterValue, RegisterAtom*);
static bool rds_s32_des(const RegisterAtom*, RegisterValue*);
static bool rds_s64_ser(const RegisterValue, RegisterAtom*);
static bool rds_s64_des(const RegisterAtom*, RegisterValue*);
static bool rds_f32_ser(const RegisterValue, RegisterAtom*);
static bool rds_f32_des(const RegisterAtom*, RegisterValue*);

/* Validators */
static inline bool rv_check_max_value(const RegisterValueU, const RegisterValue);
static inline bool rv_check_min_value(const RegisterValueU, const RegisterValue);
static inline bool rv_check_min(RegisterEntry*, const RegisterValue);
static inline bool rv_check_max(RegisterEntry*, const RegisterValue);
static inline bool rv_check_range(RegisterEntry*, const RegisterValue);
static inline bool rv_check_cb(RegisterEntry*, const RegisterValue);
static bool rv_validate(RegisterEntry*, const RegisterValue);

/* Initialisation utilities */
static inline bool is_end_of_areas(RegisterArea*);
static inline bool is_end_of_entries(RegisterEntry*);
static AreaHandle reg_count_areas(RegisterArea*);
static RegisterHandle reg_count_entries(RegisterEntry*);
static RegisterHandle ra_first_entry_of_next(RegisterTable*,
                                             RegisterArea*,
                                             RegisterHandle);

/* Area utilities */
static inline bool register_area_can_write(const RegisterArea*);
static inline bool register_area_is_writeable(const RegisterArea*);
static inline bool register_area_is_readable(const RegisterArea*);
static bool ra_addr_is_part_of(RegisterArea*, RegisterAddress);
static inline bool ra_reg_is_part_of(RegisterArea*, RegisterEntry*);
static bool ra_reg_fits_into(RegisterArea*, RegisterEntry*);
static AreaHandle ra_find_area_by_addr(RegisterTable*, RegisterAddress);
static inline int ra_range_touches(RegisterArea*,
                                   RegisterAddress,
                                   RegisterOffset);

/* Entry utilities */
static void reg_taint_in_range(RegisterTable*, RegisterAddress, RegisterOffset);
static bool reg_entry_is_in_memory(RegisterTable*, RegisterEntry*);
static inline void reg_read_entry(RegisterEntry*, RegisterAtom*);
static inline int reg_range_touches(RegisterEntry*,
                                    RegisterAddress,
                                    RegisterOffset);

/* Block write utilities */
static RegisterAccess ra_writeable(RegisterTable*,
                                   RegisterAddress,
                                   RegisterOffset);
RegisterAccess ra_malformed_write(RegisterTable*,
                                  RegisterAddress,
                                  RegisterOffset,
                                  RegisterAtom*);

/* Miscellaneous Utilities */

static bool reg_is_hexstr(const char*, size_t);
static RegisterAtom reg_c2a(int);
static RegisterAtom reg_atom_from_hexstr(const char*, size_t);

/*
 * Internal API implementation
 */

static inline size_t
reg_min(size_t a, size_t b)
{
    return (a > b) ? b : a;
}

static bool
rds_invalid_ser(const RegisterValue v, RegisterAtom *r)
{
    (void)v;
    (void)r;
    assert(false);
    return false;
}

static bool
rds_invalid_des(const RegisterAtom *r, RegisterValue *v)
{
    (void)r;
    (void)v;
    assert(false);
    return false;
}

static bool
rds_u16_ser(const RegisterValue v, RegisterAtom *r)
{
    assert(v.type == REG_TYPE_UINT16);
    *r = v.value.u16;
    return true;
}

static bool
rds_u16_des(const RegisterAtom *r, RegisterValue *v)
{
    uint16_t n = *r;
    v->value.u16 = n;
    v->type = REG_TYPE_UINT16;
    return true;
}

static bool
rds_u32_ser(const RegisterValue v, RegisterAtom *r)
{
    assert(v.type == REG_TYPE_UINT32);
    *r = v.value.u32 & 0xfffful;
    *(r+1) = (v.value.u32 >> 16u) & 0xfffful;
    return true;
}

static bool
rds_u32_des(const RegisterAtom *r, RegisterValue *v)
{
    uint32_t n = *r;
    n |= ((uint32_t)*(r+1)) << 16u;
    v->value.u32 = n;
    v->type = REG_TYPE_UINT32;
    return true;
}

static bool
rds_u64_ser(const RegisterValue v, RegisterAtom *r)
{
    assert(v.type == REG_TYPE_UINT64);
    *(r+0) = (v.value.u64 >>  0u) & 0xfffful;
    *(r+1) = (v.value.u64 >> 16u) & 0xfffful;
    *(r+2) = (v.value.u64 >> 32u) & 0xfffful;
    *(r+3) = (v.value.u64 >> 48u) & 0xfffful;
    return true;
}

static bool
rds_u64_des(const RegisterAtom *r, RegisterValue *v)
{
    uint64_t n = *r;
    n |= ((uint64_t)*(r+1)) << 16u;
    n |= ((uint64_t)*(r+2)) << 32u;
    n |= ((uint64_t)*(r+3)) << 48u;
    v->value.u64 = n;
    v->type = REG_TYPE_UINT64;
    return true;
}

static bool
rds_s16_ser(const RegisterValue v, RegisterAtom *r)
{
    assert(v.type == REG_TYPE_SINT16);
    /*
     * This assumes that the machine that this runs on uses two's complement to
     * encode negative numbers, which is the format this module uses in its re-
     * presentation. This is true for all signed-ser/des functions for now.
     */
    *r = v.value.u16;
    return true;
}

static bool
rds_s16_des(const RegisterAtom *r, RegisterValue *v)
{
    uint16_t n = *r;
    v->value.u16 = n;
    v->type = REG_TYPE_SINT16;
    return true;
}

static bool
rds_s32_ser(const RegisterValue v, RegisterAtom *r)
{
    assert(v.type == REG_TYPE_SINT32);
    *r = v.value.u32 & 0xfffful;
    *(r+1) = (v.value.u32 >> 16u) & 0xfffful;
    return true;
}

static bool
rds_s32_des(const RegisterAtom *r, RegisterValue *v)
{
    uint32_t n = *r;
    n |= ((uint32_t)*(r+1)) << 16u;
    v->value.u32 = n;
    v->type = REG_TYPE_SINT32;
    return true;
}

static bool
rds_s64_ser(const RegisterValue v, RegisterAtom *r)
{
    assert(v.type == REG_TYPE_SINT64);
    *(r+0) = (v.value.u64 >>  0u) & 0xfffful;
    *(r+1) = (v.value.u64 >> 16u) & 0xfffful;
    *(r+2) = (v.value.u64 >> 32u) & 0xfffful;
    *(r+3) = (v.value.u64 >> 48u) & 0xfffful;
    return true;
}

static bool
rds_s64_des(const RegisterAtom *r, RegisterValue *v)
{
    uint64_t n = *r;
    n |= ((uint64_t)*(r+1)) << 16u;
    n |= ((uint64_t)*(r+2)) << 32u;
    n |= ((uint64_t)*(r+3)) << 48u;
    v->value.u64 = n;
    v->type = REG_TYPE_SINT64;
    return true;
}

static bool
rds_f32_ser(const RegisterValue v, RegisterAtom *r)
{
    assert(v.type == REG_TYPE_FLOAT32);
    /* Here is another assumption: The register table is supposed to represent
     * 32 bit floating point numbers in IEEE754 format. And this assumes that
     * the target architecture uses that one as well. */
    if ((v.value.f32 != 0.) && (isnormal(v.value.f32) == false))
        return false;
    *r = v.value.u32 & 0xfffful;
    *(r+1) = (v.value.u32 >> 16u) & 0xfffful;
    return true;
}

static bool
rds_f32_des(const RegisterAtom *r, RegisterValue *v)
{
    uint32_t n = *r;
    n |= ((uint32_t)*(r+1)) << 16u;
    v->value.u32 = n;
    v->type = REG_TYPE_FLOAT32;
    return ((v->value.f32 == 0.) || (isnormal(v->value.f32) == true));
}

#define rs(t) (sizeof(t) / sizeof(RegisterAtom))

const RegisterSerDes rds_serdes[] = {
    [REG_TYPE_INVALID] = { rds_invalid_ser, rds_invalid_des, 0 },
    [REG_TYPE_UINT16] = { rds_u16_ser, rds_u16_des, rs(uint16_t) },
    [REG_TYPE_UINT32] = { rds_u32_ser, rds_u32_des, rs(uint32_t) },
    [REG_TYPE_UINT64] = { rds_u64_ser, rds_u64_des, rs(uint64_t) },
    [REG_TYPE_SINT16] = { rds_s16_ser, rds_s16_des, rs(int16_t) },
    [REG_TYPE_SINT32] = { rds_s32_ser, rds_s32_des, rs(int32_t) },
    [REG_TYPE_SINT64] = { rds_s64_ser, rds_s64_des, rs(int64_t) },
    [REG_TYPE_FLOAT32] = { rds_f32_ser, rds_f32_des, rs(float) }
};

static inline bool
rv_check_min_value(const RegisterValueU limit, const RegisterValue v)
{
    switch (v.type) {
    case REG_TYPE_INVALID:
        return false;
    case REG_TYPE_UINT16:
        return (v.value.u16 >= limit.u16);
    case REG_TYPE_UINT32:
        return (v.value.u32 >= limit.u32);
    case REG_TYPE_UINT64:
        return (v.value.u64 >= limit.u64);
    case REG_TYPE_SINT16:
        return (v.value.s16 >= limit.s16);
    case REG_TYPE_SINT32:
        return (v.value.s32 >= limit.s32);
    case REG_TYPE_SINT64:
        return (v.value.s64 >= limit.s64);
    case REG_TYPE_FLOAT32:
        return (v.value.f32 >= limit.f32);
    default:
        return false;
    }
}

static inline bool
rv_check_min(RegisterEntry *e, const RegisterValue v)
{
    return rv_check_min_value(e->check.arg.min, v);
}

static inline bool
rv_check_max_value(const RegisterValueU limit, const RegisterValue v)
{
    switch (v.type) {
    case REG_TYPE_INVALID:
        return false;
    case REG_TYPE_UINT16:
        return (v.value.u16 <= limit.u16);
    case REG_TYPE_UINT32:
        return (v.value.u32 <= limit.u32);
    case REG_TYPE_UINT64:
        return (v.value.u64 <= limit.u64);
    case REG_TYPE_SINT16:
        return (v.value.s16 <= limit.s16);
    case REG_TYPE_SINT32:
        return (v.value.s32 <= limit.s32);
    case REG_TYPE_SINT64:
        return (v.value.s64 <= limit.s64);
    case REG_TYPE_FLOAT32:
        return (v.value.f32 <= limit.f32);
    default:
        return false;
    }
}

static inline bool
rv_check_max(RegisterEntry *e, const RegisterValue v)
{
    return rv_check_max_value(e->check.arg.max, v);
}

static inline bool
rv_check_range(RegisterEntry *e, const RegisterValue v)
{
    return (rv_check_min_value(e->check.arg.range.min, v) &&
            rv_check_max_value(e->check.arg.range.max, v));
}

static inline bool
rv_check_cb(RegisterEntry *e, const RegisterValue v)
{
    return e->check.arg.cb(e, v);
}

static bool
rv_validate(RegisterEntry *e, const RegisterValue v)
{
    if (e->type != v.type)
        return false;

    switch (e->check.type) {
    case REGV_TYPE_TRIVIAL:
        return true;
    case REGV_TYPE_MIN:
        return rv_check_min(e, v);
    case REGV_TYPE_MAX:
        return rv_check_max(e, v);
    case REGV_TYPE_RANGE:
        return rv_check_range(e, v);
    case REGV_TYPE_CALLBACK:
        return rv_check_cb(e, v);
    default:
        return false;
    }
}

static inline bool
is_end_of_areas(RegisterArea *a)
{
    if (a->read != NULL || a->write != NULL)
        return false;
    if (a->size != 0 || a->base != 0)
        return false;
    if (a->mem != NULL)
        return false;
    return true;
}

static inline int
reg_range_touches(RegisterEntry *e, RegisterAddress addr, RegisterOffset n)
{
    /* Return -1 if entry is below range; 0 if it is within the range and 1 if
     * it is above the range */
    const RegisterOffset size = rds_serdes[e->type].size;

    if ((e->address + size) <= addr)
        return -1;

    if ((addr + n) <= e->address)
        return 1;

    return 0;
}

static void
reg_taint_in_range(RegisterTable *t, RegisterAddress addr, RegisterOffset n)
{
    for (RegisterOffset i = 0ul; i < t->entries; ++i) {
        int touch = reg_range_touches(&t->entry[i], addr, n);
        if (touch > 0)
            return;
        if (touch < 0)
            continue;
        register_touch(t, i);
    }
}

static AreaHandle
reg_count_areas(RegisterArea *a)
{
    AreaHandle n = 0ull;
    while ((n < AREA_HANDLE_MAX) && (is_end_of_areas(a) == false)) {
        n++;
        a++;
    }
    return n;
}

static inline bool
is_end_of_entries(RegisterEntry *e)
{
    return (e->type == REG_TYPE_INVALID);
}

static RegisterHandle
reg_count_entries(RegisterEntry *e)
{
    RegisterHandle n = 0ull;
    while ((n < REGISTER_HANDLE_MAX) && (is_end_of_entries(e) == false)) {
        n++;
        e++;
    }
    return n;
}

static inline bool
register_area_can_write(const RegisterArea *a)
{
    return (a->write != NULL);
}

static inline bool
register_area_is_writeable(const RegisterArea *a)
{
    return (register_area_can_write(a) && BIT_ISSET(a->flags,REG_AF_WRITEABLE));
}

static inline bool
register_area_is_readable(const RegisterArea *a)
{
    return ((a->read != NULL) && (BIT_ISSET(a->flags, REG_AF_READABLE)));
}

static bool
ra_addr_is_part_of(RegisterArea *a, RegisterAddress addr)
{
    if (a->base > addr)
        return false;

    if ((a->base + a->size) <= addr)
        return false;

    return true;
}

static inline bool
ra_reg_is_part_of(RegisterArea *a, RegisterEntry *e)
{
    return ra_addr_is_part_of(a, e->address);
}

static bool
ra_reg_fits_into(RegisterArea *a, RegisterEntry *e)
{
    const RegisterAddress area_end = a->base + a->size;
    const RegisterAddress entry_end = e->address + rds_serdes[e->type].size;
    return (entry_end <= area_end);
}

static AreaHandle
ra_find_area_by_addr(RegisterTable *t, RegisterAddress addr)
{
    AreaHandle n;
    for (n = 0ull; n < t->areas; ++n) {
        if (ra_addr_is_part_of(&t->area[n], addr)) {
            break;
        }
    }

    return n;
}

static bool
reg_entry_is_in_memory(RegisterTable *t, RegisterEntry *e)
{
    for (AreaHandle an = 0ul; an < t->areas; ++an) {
        RegisterArea *area = &t->area[an];
        if (ra_reg_is_part_of(area, e)) {
            if (ra_reg_fits_into(area, e) == false)
                return false;

            e->area = area;
            e->offset = e->address - area->base;
            return true;
        }
    }
    return false;
}

static inline void
reg_read_entry(RegisterEntry *e, RegisterAtom *buf)
{
    (void)e->area->read(e->area, buf, e->offset, rds_serdes[e->type].size);
}

static inline int
ra_range_touches(RegisterArea *a, RegisterAddress addr, RegisterOffset n)
{
    /* Return -1 if area is below range; 0 if it is within the range and 1 if
     * it is above the range */
    if ((a->base + a->size) <= addr)
        return -1;

    if ((addr + n) <= a->base)
        return 1;

    return 0;
}
static RegisterHandle
ra_first_entry_of_next(RegisterTable *t, RegisterArea *a, RegisterHandle start)
{
    for (RegisterHandle i = start; i < t->entries; ++i) {
        if (ra_addr_is_part_of(a, t->entry[i].address) == false)
            return i;
    }
    return t->entries;
}

static RegisterAccess
ra_writeable(RegisterTable *t, RegisterAddress addr, RegisterOffset n)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    /*
     * There are two kinds of "writeability":
     *
     * - An area does not have the REG_AF_WRITEABLE bit set in its flags field.
     *   This means that the outside may not write into the memory range that
     *   the area controls.
     *
     * - The area does not define a write() callback. This means that it's not
     *   at all possible for the register abstraction to modify the range of
     *   memory in question.
     */
    for (AreaHandle i = 0ul; i < t->areas; ++i) {
        int touch = ra_range_touches(&t->area[i], addr, n);
        if (touch < 0)
            continue;
        if (touch > 0)
            break;
        if (register_area_is_writeable(&t->area[i]) == false) {
            rv.code = REG_ACCESS_READONLY;
            rv.address = addr;
            return rv;
        }
    }

    return rv;
}

RegisterAccess
ra_malformed_write(RegisterTable *t, RegisterAddress addr,
                   RegisterOffset n, RegisterAtom *buf)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    RegisterAddress last = addr + n - 1;

    for (RegisterHandle i = 0ul; i < t->entries; ++i) {
        RegisterEntry *e = &t->entry[i];
        RegisterValue datum;
        RegisterAtom raw[REG_SIZEOF_LARGEST_DATUM];
        const RegisterOffset size = rds_serdes[e->type].size;
        const RegisterAddress end = e->address + size - 1;
        RegisterAddress bs, rs;
        RegisterOffset rlen;

        /* Skip entries before block start */
        if (addr > end)
            continue;

        /* Terminate for entries after last */
        if (e->address > last)
            break;

        /*
         * What we now need to do is this: Which parts of the entry does the
         * block touch. There are four cases:
         *
         * 1. The block touches the entire entry.
         * 2. The block only touches end of the entry.
         * 3. The block only touches the beginning of the entry.
         * 4. The block only touches an embedded part of the entry (2+3).
         *
         * Case 2 can only happen with the first entry that is processed. And
         * indeed, the first atom of the block has to be used. Conversely, case
         * 3 can only happen with the last entry being processed. And indeed,
         * that last atom of the block has to be involved.
         *
         * Case 4 is a combination of 2 and three. Thus the entry being proces-
         * sed has to be first as well as the last and therefore the only entry
         * that's touched.
         *
         * Case 1 covers all the other cases, where all the atoms of an entry
         * have to be touched while processing.
         *
         * Now we need to figure out which atoms of the block map to the entry
         * we're working on: This is simple though because the new block has a
         * starting address and a length and every entry knows to which address
         * in the register-table's address space it is mapped to.
         *
         * That means we can just substract the block start address from the
         * entry's address and have the offset from the block start to find the
         * atoms that should go into the newly formed binary form of the
         * register table entry.
         *
         * That with the information about which parts are touched tells us
         * precisely which atoms from the new block should go into the entry's
         * memory. Perform that in temporary memory.
         */

        rlen = size;
        if (addr > e->address) {
            /* This can only happen with the first entry the block touches. */
            bs = 0ull;
            rs = addr - e->address;
            rlen -= rs - 1;
        } else {
            bs = e->address - addr;
            rs = 0ull;
        }

        if (end > last) {
            /* This can only happen with the last entry the block touches. */
            rlen -= bs + size - n;
        }

        /* Fetch the entire memory of where the old entry is stored */
        reg_read_entry(e, raw);
        memcpy(raw + rs, buf + bs, rlen * sizeof(RegisterAtom));

        /* Try the deserialiser, fail if it fails */
        if (rds_serdes[e->type].des(raw, &datum) == false) {
            rv.code = REG_ACCESS_INVALID;
            rv.address = addr + bs;
            return rv;
        }

        /* Try the validator, fail if it fails */
        if (rv_validate(e, datum) == false) {
            rv.code = REG_ACCESS_RANGE;
            rv.address = addr + bs;
            return rv;
        }
    }
    return rv;
}

static bool
reg_is_hexstr(const char *s, const size_t n)
{
    for (size_t idx = 0u; idx < n; ++idx) {
        if (isxdigit((int)s[idx]) == false)
            return false;
    }

    return true;
}

static RegisterAtom
reg_c2a(int c)
{
    switch (c) {
    case '1': return 1u;
    case '2': return 2u;
    case '3': return 3u;
    case '4': return 4u;
    case '5': return 5u;
    case '6': return 6u;
    case '7': return 7u;
    case '8': return 8u;
    case '9': return 9u;
    case 'a': return 10u;
    case 'b': return 11u;
    case 'c': return 12u;
    case 'd': return 13u;
    case 'e': return 14u;
    case 'f': return 15u;
    default: return 0u;
    }
}

static RegisterAtom
reg_atom_from_hexstr(const char *s, const size_t n)
{
    RegisterAtom rv = 0u;

    for (size_t idx = 0u; idx < n; ++idx) {
        const size_t shift = (n-idx-1) * 4u;
        const RegisterAtom v = reg_c2a(tolower((int)s[idx]));
        rv |= (v & 0x0fu) << shift;
    }

    return rv;
}

/* Public API */

RegisterInit
register_init(RegisterTable *t)
{
    RegisterInit rv = REG_INIT_RESULT_INIT;
    RegisterAddress previous;

    if (t == NULL) {
        rv.code = REG_INIT_TABLE_INVALID;
        return rv;
    }

    if (t->area == NULL) {
        rv.code = REG_INIT_TABLE_INVALID;
        return rv;
    }

    if (t->entry == NULL) {
        rv.code = REG_INIT_TABLE_INVALID;
        return rv;
    }

    BIT_CLEAR(t->flags, REG_TF_INITIALISED);
    /* Determine table sizes first */
    t->areas = reg_count_areas(t->area);
    if (t->areas == AREA_HANDLE_MAX) {
        rv.code = REG_INIT_TOO_MANY_AREAS;
        rv.pos.area = AREA_HANDLE_MAX;
        return rv;
    }
    t->entries = reg_count_entries(t->entry);
    if (t->entries == REGISTER_HANDLE_MAX) {
        rv.code = REG_INIT_TOO_MANY_ENTRIES;
        rv.pos.entry = REGISTER_HANDLE_MAX;
        return rv;
    }

    if (t->areas == 0ul) {
        rv.code = REG_INIT_NO_AREAS;
        rv.pos.area = 0;
        return rv;
    }

    previous = t->area[0].base;
    for (AreaHandle i = 1ul; i < t->areas; ++i) {
        const RegisterAddress current = t->area[i].base;
        if (current < previous) {
            rv.code = REG_INIT_AREA_INVALID_ORDER;
            rv.pos.area = i;
            return rv;
        }
        if (current < (previous + t->area[i-1].size)) {
            rv.code = REG_INIT_AREA_ADDRESS_OVERLAP;
            rv.pos.area = i;
            return rv;
        }
        previous = current;
    }

    previous = t->entry[0].address;
    for (RegisterHandle i = 1ul; i < t->entries; ++i) {
        const RegisterAddress current = t->entry[i].address;
        if (t->entry[i].address < previous) {
            rv.code = REG_INIT_ENTRY_INVALID_ORDER;
            rv.pos.entry = i;
            return rv;
        }
        if (current < (previous+rds_serdes[t->entry[i-1].type].size)) {
            rv.code = REG_INIT_ENTRY_ADDRESS_OVERLAP;
            rv.pos.entry = i;
            return rv;
        }
        previous = current;
    }

    /*
     * Looks like TI's C99 compiler doesn't initialise arrays that are initia-
     * lised like this
     *
     *   int foo[32] = { 0 };
     *
     * correctly. Only initialising the first item of the array. Initialise all
     * memory to zero upon boot then...
     */
    for (AreaHandle i = 0ul; i < t->areas; ++i) {
        memset(t->area[i].mem, 0, t->area[i].size * sizeof(RegisterAtom));
    }

    BIT_SET(t->flags, REG_TF_INITIALISED);
    for (RegisterHandle i = 0ul; i < t->entries; ++i) {
        RegisterAccess access;
        RegisterValue def;
        RegisterEntry *e = &t->entry[i];
        /* Link into register table memory */
        bool success = reg_entry_is_in_memory(t, e);
        if (success == false) {
            rv.code = REG_INIT_ENTRY_IN_MEMORY_HOLE;
            rv.pos.entry = i;
            BIT_CLEAR(t->flags, REG_TF_INITIALISED);
            return rv;
        }
        e->area = &t->area[ra_find_area_by_addr(t, e->address)];

        /* Load default value into register table */
        def.value = e->default_value;
        def.type = e->type;
        access = register_set(t, i, def);

        if (access.code != REG_ACCESS_SUCCESS) {
            rv.code = REG_INIT_ENTRY_INVALID_DEFAULT;
            rv.pos.entry = i;
            BIT_CLEAR(t->flags, REG_TF_INITIALISED);
            return rv;
        }
    }

    /* Now link entries back into their area (first and last) */
    RegisterHandle entry = 0ul;
    for (AreaHandle i = 0ul; i < t->areas; ++i) {
        RegisterArea *a = &t->area[i];
        RegisterEntry *e = &t->entry[entry];
        /* The area and entry lists of a table are sorted by address and all
         * entries have to map to an area. Therefore, when we're trying to find
         * the first entry of an area is either the trivial case or the area is
         * empty and does not contain any entry at all. */
        if (entry < t->entries && ra_addr_is_part_of(a, e->address)) {
            a->entry.first = entry;
            entry = ra_first_entry_of_next(t, a, entry + 1u);
            a->entry.last = entry - 1;
            a->entry.count = entry - a->entry.first;
        } else {
            a->entry.first = a->entry.last = a->entry.count = 0;
        }
    }

    return rv;
}

RegisterAccess
register_set(RegisterTable *t, RegisterHandle idx, const RegisterValue v)
{
    RegisterAtom raw[REG_SIZEOF_LARGEST_DATUM];
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    RegisterEntry *e;
    RegisterArea *a;
    bool success;

    if (BIT_ISSET(t->flags, REG_TF_INITIALISED) == false) {
        rv.code = REG_ACCESS_UNINITIALISED;
        rv.address = idx;
        return rv;
    }

    if (idx > t->entries) {
        rv.code = REG_ACCESS_NOENTRY;
        rv.address = idx;
        return rv;
    }

    e = &t->entry[idx];
    success = rv_validate(e, v);
    if (success == false) {
        rv.code = REG_ACCESS_RANGE;
        rv.address = e->address;
        return rv;
    }

    a = e->area;

    if (register_area_can_write(a) == false) {
        rv.code = REG_ACCESS_READONLY;
        rv.address = e->address;
        return rv;
    }

    success = rds_serdes[e->type].ser(v, raw);

    if (success == false) {
        rv.code = REG_ACCESS_INVALID;
        rv.address = e->address;
        return rv;
    }

    a->write(a, raw, e->offset, rds_serdes[e->type].size);
    return rv;
}

RegisterAccess
register_get(RegisterTable *t, RegisterHandle idx, RegisterValue *v)
{
    RegisterAtom raw[REG_SIZEOF_LARGEST_DATUM];
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    RegisterEntry *e;
    RegisterArea *a;
    bool success;

    if (BIT_ISSET(t->flags, REG_TF_INITIALISED) == false) {
        rv.code = REG_ACCESS_UNINITIALISED;
        rv.address = idx;
        return rv;
    }

    if (idx >= t->entries) {
        rv.code = REG_ACCESS_NOENTRY;
        rv.address = idx;
        return rv;
    }

    e = &t->entry[idx];
    a = e->area;
    a->read(a, raw, e->offset, rds_serdes[e->type].size);
    success = rds_serdes[e->type].des(raw, v);

    if (success == false) {
        rv.code = REG_ACCESS_INVALID;
        rv.address = idx;
    }
    return rv;
}

RegisterAccess
register_default(RegisterTable *t, RegisterHandle idx, RegisterValue *v)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    RegisterEntry *e;

    if (BIT_ISSET(t->flags, REG_TF_INITIALISED) == false) {
        rv.code = REG_ACCESS_UNINITIALISED;
        rv.address = idx;
        return rv;
    }

    if (idx >= t->entries) {
        rv.code = REG_ACCESS_NOENTRY;
        rv.address = idx;
        return rv;
    }

    e = &t->entry[idx];
    v->type = e->type;
    v->value = e->default_value;

    return rv;
}

/* These _unsafe() functions are marked unsafe for a reason. You should make
 * sure before using it, that the block access they are asked to do is NOT
 * going to try to touch any memory holes! */
void
register_block_read_unsafe(RegisterTable *t, RegisterAddress addr,
                           RegisterOffset n, RegisterAtom *buf)
{
    RegisterOffset rest = n;
    while (rest > 0ull) {
        AreaHandle an = ra_find_area_by_addr(t, addr);
        RegisterOffset offset, readn;
        RegisterArea *a;

        assert(an < t->areas);
        a = &t->area[an];
        offset = addr - a->base;
        readn = reg_min(a->base + a->size - addr, rest);

        if (register_area_is_readable(a)) {
            a->read(a, buf, offset, readn);
        } else {
            /* Memory that can't be read reads back zeroes */
            memset(buf + offset, 0, sizeof(RegisterAtom) * readn);
        }

        buf += readn;
        addr += readn;
        rest -= readn;
    }
}

void
register_block_write_unsafe(RegisterTable *t, RegisterAddress addr,
                            RegisterOffset n, RegisterAtom *buf)
{
    RegisterOffset rest = n;
    while (rest > 0ull) {
        AreaHandle an = ra_find_area_by_addr(t, addr);
        RegisterOffset offset, writen;
        RegisterArea *a;

        assert(an < t->areas);
        a = &t->area[an];
        offset = addr - a->base;
        writen = reg_min(a->base + a->size - addr, rest);
        a->write(a, buf, offset, writen);
        buf += writen;
        addr += writen;
        rest -= writen;
    }
}

RegisterAccess
register_block_read(RegisterTable *t, RegisterAddress addr,
                    RegisterOffset n, RegisterAtom *buf)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;

    if (BIT_ISSET(t->flags, REG_TF_INITIALISED) == false) {
        rv.code = REG_ACCESS_UNINITIALISED;
        rv.address = addr;
        return rv;
    }

    if (n == 0ull)
        return rv;

    rv = register_block_touches_hole(t, addr, n);
    if (rv.code != REG_ACCESS_SUCCESS)
        return rv;

    register_block_read_unsafe(t, addr, n, buf);
    return rv;
}

RegisterAccess
register_block_write(RegisterTable *t, RegisterAddress addr,
                     RegisterOffset n, RegisterAtom *buf)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;

    if (BIT_ISSET(t->flags, REG_TF_INITIALISED) == false) {
        rv.code = REG_ACCESS_UNINITIALISED;
        rv.address = addr;
        return rv;
    }

    /* Zero-length writes finish trivially. */
    if (n == 0ull)
        return rv;

    rv = ra_writeable(t, addr, n);
    if (rv.code != REG_ACCESS_SUCCESS)
        return rv;

    /* Make sure the block write instruction does not want to write into
     * an address that does not map to an area in the register table. */

    rv = register_block_touches_hole(t, addr, n);
    if (rv.code != REG_ACCESS_SUCCESS)
        return rv;

    rv = ra_malformed_write(t, addr, n, buf);
    if (rv.code != REG_ACCESS_SUCCESS)
        return rv;

    /* If the previous validation steps succeeded, it is safe to push this
     * chunk of memory into the referenced register table. Since we checked for
     * all errors before hand, it is safe to use this function. */

    register_block_write_unsafe(t, addr, n, buf);
    reg_taint_in_range(t, addr, n);
    return rv;
}

RegisterAccess
reg_mem_read(const RegisterArea *a, RegisterAtom *dest,
             RegisterOffset offset, RegisterOffset n)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    memcpy(dest, a->mem + offset, n * sizeof(RegisterAtom));
    return rv;
}

RegisterAccess
reg_mem_write(RegisterArea *a, const RegisterAtom *src,
              RegisterOffset offset, RegisterOffset n)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    memcpy(a->mem + offset, src, n * sizeof(RegisterAtom));
    return rv;
}

RegisterAccess
register_block_touches_hole(RegisterTable *t, RegisterAddress addr,
                            RegisterOffset n)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    RegisterOffset rest = n;
    while (rest > 0) {
        RegisterArea *a;
        RegisterOffset used;
        AreaHandle an = ra_find_area_by_addr(t, addr);

        if (an == t->areas) {
            rv.code = REG_ACCESS_NOENTRY;
            rv.address = addr;
            return rv;
        }

        a = &t->area[an];
        used = reg_min(a->base + a->size - addr, rest);
        rest -= used;
        addr += used;
    }
    return rv;
}

RegisterAccess
register_set_from_hexstr(RegisterTable *t, const RegisterAddress start,
                         const char *str, const size_t n)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;

    for (size_t idx = 0; idx < n; idx += 4u) {
        const char *cur = str+idx;
        const size_t cn = reg_min(4, n - idx);
        const RegisterAddress ca = start + idx/4u;

        const AreaHandle ah = ra_find_area_by_addr(t, ca);
        if (ah >= t->areas) {
            rv.code = REG_ACCESS_NOENTRY;
            rv.address = ca;
            return rv;
        }

        const RegisterArea *area = &t->area[ah];
        if (register_area_can_write(area) == false) {
            rv.code = REG_ACCESS_READONLY;
            rv.address = ca;
            return rv;
        }

        if (reg_is_hexstr(cur, cn) == false) {
            rv.code = REG_ACCESS_INVALID;
            rv.address = ca;
            return rv;
        }

        const RegisterAtom value = reg_atom_from_hexstr(cur, cn);
        const RegisterOffset o = ca - area->base;
        const size_t size = rds_serdes[REG_TYPE_UINT16].size;
        rv = area->write((RegisterArea*)area, &value, o, size);
        if (rv.code != REG_ACCESS_SUCCESS) {
            return rv;
        }
    }

    return rv;
}
