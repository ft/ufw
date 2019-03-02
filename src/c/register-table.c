/*
 * Copyright (c) 2019 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef DEBUG
#define NDEBUG
#endif /* DEBUG */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef INC_REGISTER_TABLE_H
#include <c/register-table.h>
#endif /* INC_REGISTER_TABLE_H */

/* Internal API */

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
    return true;
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
    [REG_TYPE_FLOAT32] = { rds_f32_ser, rds_f32_des, rs(float) },
    [REG_TYPE_STRING] = { rds_invalid_ser, rds_invalid_des, 0 }
};

static inline bool
rv_check_min(RegisterEntry *e, const RegisterValue v)
{
    switch (v.type) {
    case REG_TYPE_INVALID:
        return false;
    case REG_TYPE_UINT16:
        return (v.value.u16 >= e->check.arg.min.u16);
    case REG_TYPE_UINT32:
        return (v.value.u32 >= e->check.arg.min.u32);
    case REG_TYPE_UINT64:
        return (v.value.u64 >= e->check.arg.min.u64);
    case REG_TYPE_SINT16:
        return (v.value.s16 >= e->check.arg.min.s16);
    case REG_TYPE_SINT32:
        return (v.value.s32 >= e->check.arg.min.s32);
    case REG_TYPE_SINT64:
        return (v.value.s64 >= e->check.arg.min.s64);
    case REG_TYPE_FLOAT32:
        return (v.value.f32 >= e->check.arg.min.f32);
    case REG_TYPE_STRING:
        return true;
    }
}

static inline bool
rv_check_max(RegisterEntry *e, const RegisterValue v)
{
    switch (v.type) {
    case REG_TYPE_INVALID:
        return false;
    case REG_TYPE_UINT16:
        return (v.value.u16 <= e->check.arg.max.u16);
    case REG_TYPE_UINT32:
        return (v.value.u32 <= e->check.arg.max.u32);
    case REG_TYPE_UINT64:
        return (v.value.u64 <= e->check.arg.max.u64);
    case REG_TYPE_SINT16:
        return (v.value.s16 <= e->check.arg.max.s16);
    case REG_TYPE_SINT32:
        return (v.value.s32 <= e->check.arg.max.s32);
    case REG_TYPE_SINT64:
        return (v.value.s64 <= e->check.arg.max.s64);
    case REG_TYPE_FLOAT32:
        return (v.value.f32 <= e->check.arg.max.f32);
    case REG_TYPE_STRING:
        return true;
    }
}

static inline bool
rv_check_range(RegisterEntry *e, const RegisterValue v)
{
    return (rv_check_min(e, v) && rv_check_max(e, v));
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
reg_range_touches(RegisterEntry *e, RegisterAddress addr, size_t n)
{
    /* Return -1 if entry is below range; 0 if it is within the range and 1 if
     * it is above the range */
    const size_t size = rds_serdes[e->type].size;

    if ((e->address + size) < addr)
        return -1;

    if ((addr + n) < e->address)
        return 1;

    return 0;
}

static void
reg_taint_in_range(RegisterTable *t, RegisterAddress addr, size_t n)
{
    for (size_t i = 0ull; i < t->entries; ++i) {
        int touch = reg_range_touches(&t->entry[i], addr, n);
        if (touch > 0)
            return;
        if (touch < 0)
            continue;
        BIT_SET(t->entry[i].state, REG_STATE_TOUCHED);
    }
}

static size_t
reg_count_areas(RegisterArea *a)
{
    size_t n = 0ull;
    while (is_end_of_areas(a) == false) {
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

static size_t
reg_count_entries(RegisterEntry *e)
{
    size_t n = 0ull;
    while (is_end_of_entries(e) == false) {
        n++;
        e++;
    }
    return n;
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
    const size_t area_end = a->base + a->size;
    const size_t entry_end = e->address + rds_serdes[e->type].size;
    return (area_end <= entry_end);
}

static size_t
ra_find_area_by_addr(RegisterTable *t, RegisterAddress addr)
{
    size_t n;
    for (n = 0ull; n < t->areas; ++n) {
        if (ra_addr_is_part_of(&t->area[n], addr)) {
            break;
        }
    }

    return n;
}

static bool
ra_entry_is_in_memory(RegisterTable *t, RegisterEntry *e)
{
    for (size_t an = 0ull; an < t->areas; ++an) {
        RegisterArea *area = &t->area[an];
        if (ra_reg_is_part_of(area, e)) {
            if (ra_reg_fits_into(area, e))
                return false;

            e->area = area;
            e->offset = e->address - area->base;
            return true;
        }
    }
    return false;
}

static inline void
ra_read_entry(RegisterEntry *e, RegisterAtom *buf)
{
    (void)e->area->read(e->area, buf, e->offset, rds_serdes[e->type].size);
}

static inline int
ra_range_touches(RegisterArea *a, RegisterAddress addr, size_t n)
{
    /* Return -1 if area is below range; 0 if it is within the range and 1 if
     * it is above the range */
    if ((a->base + a->size) < addr)
        return -1;

    if ((addr + n) < a->base)
        return 1;

    return 0;
}

static RegisterAccessResult
ra_writable(RegisterTable *t, RegisterAddress addr, size_t n)
{
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;
    /*
     * There are two kinds of "writability":
     *
     * - An area does not have the REG_AF_WRITEABLE bit set in its flags field.
     *   This means that the outside may not write into the memory range that
     *   the area controls.
     *
     * - The area does not define a write() callback. This means that it's not
     *   at all possible for the register abstraction to modify the range of
     *   memory in question.
     */
    for (size_t i = 0ull; i < t->areas; ++i) {
        int touch = ra_range_touches(&t->area[i], addr, n);
        if (touch < 0)
            continue;
        if (touch > 0)
            break;
        if (register_area_is_writable(&t->area[i]) == false) {
            rv.code = REG_ACCESS_READONLY;
            rv.address = addr;
            return rv;
        }
    }

    return rv;
}

RegisterAccessResult
ra_malformed_write(RegisterTable *t, RegisterAddress addr, size_t n,
                   RegisterAtom *buf)
{
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;
    RegisterAddress last = addr + n - 1;

    for (size_t i = 0ull; i < t->entries; ++i) {
        RegisterEntry *e = &t->entry[i];
        RegisterValue datum;
        RegisterAtom raw[REG_ATOM_BIGGEST_DATUM];
        const size_t size = rds_serdes[e->type].size;
        const RegisterAddress end = e->address + size - 1;
        size_t bs, rs, rlen;

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
        ra_read_entry(e, raw);
        memcpy(raw + rs, buf + bs, rlen * sizeof(RegisterAtom));

        /* Try the deserialiser, fail if it fails */
        if (rds_serdes[e->type].des(raw, &datum) == false) {
            rv.code = REG_ACCESS_INVALID;
            rv.address = addr + bs;
            return rv;
        }

        /* Try the validator, fail if it fails */
        if (rv_validate(e, datum) == false) {
            rv.code = REG_ACCESS_INVALID;
            rv.address = addr + bs;
            return rv;
        }
    }
    return rv;
}

/* Public API */

RegisterAccessResult
registers_init(RegisterTable *t)
{
    RegisterAtom raw[REG_ATOM_BIGGEST_DATUM];
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;

    /* Determine table sizes first */
    t->areas = reg_count_areas(t->area);
    t->entries = reg_count_entries(t->entry);

    for (size_t i = 0ull; i < t->entries; ++i) {
        RegisterValue def;
        RegisterEntry *e = &t->entry[i];
        /* Link into register table memory */
        bool success = ra_entry_is_in_memory(t, e);
        if (success == false) {
            rv.code = REG_ACCESS_NOENTRY;
            rv.address = e->address;
            return rv;
        }
        /* Load default value into register table */
        def.value = e->default_value;
        def.type = e->type;
        success = rds_serdes[e->type].ser(def, raw);

        if (success == false) {
            rv.code = REG_ACCESS_INVALID;
            rv.address = e->address;
            return rv;
        }
        e->area->write(e->area, raw, e->offset, rds_serdes[e->type].size);
    }

    return rv;
}

RegisterAccessResult
register_set(RegisterTable *t, size_t idx, const RegisterValue v)
{
    RegisterAtom raw[REG_ATOM_BIGGEST_DATUM];
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;
    RegisterEntry *e;
    RegisterArea *a;
    bool success;

    if (idx > t->entries) {
        rv.code = REG_ACCESS_NOENTRY;
        rv.address = idx;
        return rv;
    }

    e = &t->entry[idx];
    success = rv_validate(e, v);
    if (success == false) {
        rv.code = REG_ACCESS_INVALID;
        rv.address = e->address;
        return rv;
    }

    a = e->area;
    success = rds_serdes[e->type].ser(v, raw);

    if (success == false) {
        rv.code = REG_ACCESS_INVALID;
        rv.address = e->address;
        return rv;
    }

    a->write(a, raw, e->offset, rds_serdes[e->type].size);
    return rv;
}

RegisterAccessResult
register_get(RegisterTable *t, size_t idx, RegisterValue *v)
{
    RegisterAtom raw[REG_ATOM_BIGGEST_DATUM];
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;
    RegisterEntry *e;
    RegisterArea *a;
    bool success;

    if (idx > t->entries) {
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

RegisterAccessResult
register_default(RegisterTable *t, size_t idx, RegisterValue *v)
{
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;
    RegisterEntry *e;

    if (idx > t->entries) {
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
register_block_read_unsafe(RegisterTable *t, RegisterAddress addr, size_t n,
                           RegisterAtom *buf)
{
    size_t rest = n;
    while (rest > 0ull) {
        size_t an = ra_find_area_by_addr(t, addr);
        size_t offset, readn;
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
register_block_write_unsafe(RegisterTable *t, RegisterAddress addr, size_t n,
                            RegisterAtom *buf)
{
    size_t rest = n;
    while (rest > 0ull) {
        size_t an = ra_find_area_by_addr(t, addr);
        size_t offset, writen;
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

RegisterAccessResult
register_block_read(RegisterTable *t, RegisterAddress addr, size_t n,
                    RegisterAtom *buf)
{
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;

    if (n == 0ull)
        return rv;

    rv = register_block_touches_hole(t, addr, n);
    if (rv.code != REG_ACCESS_SUCCESS)
        return rv;

    register_block_read_unsafe(t, addr, n, buf);
    return rv;
}

RegisterAccessResult
register_block_write(RegisterTable *t, RegisterAddress addr, size_t n,
                     RegisterAtom *buf)
{
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;

    /* Zero-length writes finish trivially. */
    if (n == 0ull)
        return rv;

    rv = ra_writable(t, addr, n);
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

RegisterAccessResult
reg_mem_read(const RegisterArea *a, RegisterAtom *dest,
             RegisterOffset offset, size_t n)
{
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;
    memcpy(dest, a->mem + offset, n * sizeof(RegisterAtom));
    return rv;
}

RegisterAccessResult
reg_mem_write(RegisterArea *a, const RegisterAtom *src,
              RegisterOffset offset, size_t n)
{
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;
    memcpy(a->mem + offset, src, n * sizeof(RegisterAtom));
    return rv;
}

RegisterAccessResult
register_block_touches_hole(RegisterTable *t, RegisterAddress addr, size_t n)
{
    RegisterAccessResult rv = REG_ACCESS_RESULT_INIT;
    size_t rest = n;
    while (rest > 0) {
        RegisterArea *a;
        size_t used;
        size_t an = ra_find_area_by_addr(t, addr);

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
