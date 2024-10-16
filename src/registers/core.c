/*
 * Copyright (c) 2019-2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/binary-format.h>
#include <ufw/compiler.h>
#include <ufw/register-table.h>

#include "internal.h"

/*
 * Internal API prototypes
 */

/* Ser/Des */
static bool rds_invalid_ser(RegisterValue, RegisterAtom*, bool);
static bool rds_invalid_des(const RegisterAtom*, RegisterValue*, bool);
static bool rds_u16_ser(RegisterValue, RegisterAtom*, bool);
static bool rds_u16_des(const RegisterAtom*, RegisterValue*, bool);
static bool rds_u32_ser(RegisterValue, RegisterAtom*, bool);
static bool rds_u32_des(const RegisterAtom*, RegisterValue*, bool);
static bool rds_u64_ser(RegisterValue, RegisterAtom*, bool);
static bool rds_u64_des(const RegisterAtom*, RegisterValue*, bool);
static bool rds_s16_ser(RegisterValue, RegisterAtom*, bool);
static bool rds_s16_des(const RegisterAtom*, RegisterValue*, bool);
static bool rds_s32_ser(RegisterValue, RegisterAtom*, bool);
static bool rds_s32_des(const RegisterAtom*, RegisterValue*, bool);
static bool rds_s64_ser(RegisterValue, RegisterAtom*, bool);
static bool rds_s64_des(const RegisterAtom*, RegisterValue*, bool);
static bool rds_f32_ser(RegisterValue, RegisterAtom*, bool);
static bool rds_f32_des(const RegisterAtom*, RegisterValue*, bool);
static bool rds_f64_ser(RegisterValue, RegisterAtom*, bool);
static bool rds_f64_des(const RegisterAtom*, RegisterValue*, bool);

/* Validators */
static inline bool rv_check_max_value(RegisterValueU, RegisterValue);
static inline bool rv_check_min_value(RegisterValueU, RegisterValue);
static inline bool rv_check_min(RegisterEntry*, RegisterValue);
static inline bool rv_check_max(RegisterEntry*, RegisterValue);
static inline bool rv_check_range(RegisterEntry*, RegisterValue);
static inline bool rv_check_cb(RegisterEntry*, RegisterValue);
static bool rv_validate(RegisterTable*, RegisterEntry*, RegisterValue);

/* Initialisation utilities */
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
static inline RegisterAccess reg_read_entry(RegisterEntry*, RegisterAtom*);
static inline int reg_range_touches(RegisterEntry*,
                                    RegisterAddress,
                                    RegisterOffset);
static RegisterAccess reg_entry_sane(RegisterTable*, RegisterHandle);
static RegisterAccess reg_entry_load_default(RegisterTable*, RegisterHandle);

static RegisterAccess register_setx(RegisterTable*, RegisterHandle,
                                    RegisterValue, bool);

/* Block write utilities */
static RegisterAccess ra_writeable(RegisterTable*,
                                   RegisterAddress,
                                   RegisterOffset);
RegisterAccess ra_malformed_write(RegisterTable*,
                                  RegisterAddress,
                                  RegisterOffset,
                                  RegisterAtom*);

/* Iteration */

struct maybe_area {
    bool valid;
    AreaHandle handle;
};

struct maybe_register {
    bool valid;
    RegisterHandle handle;
};

static struct maybe_area find_area(const RegisterTable*,
                                   AreaHandle, AreaHandle,
                                   RegisterAddress);
static struct maybe_register find_reg(const RegisterTable*t,
                                      RegisterHandle, RegisterHandle,
                                      RegisterAddress);
static RegisterAccess reg_iterate(RegisterTable*,
                                  RegisterHandle, RegisterAddress,
                                  registerCallback, void*);

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
rds_invalid_ser(const RegisterValue v, RegisterAtom *r, bool be) /* NOLINT */
{
    (void)v;
    (void)r;
    (void)be;
    assert(false);
    return false;
}

static bool
rds_invalid_des(const RegisterAtom *r, RegisterValue *v, bool be)
{
    (void)r;
    (void)v;
    (void)be;
    assert(false);
    return false;
}

static bool
rds_u16_ser(const RegisterValue v, RegisterAtom *r, const bool bigendian)
{
    assert(v.type == REG_TYPE_UINT16);
    if (bigendian) {
        bf_set_u16b(r, v.value.u16);
    } else {
        bf_set_u16l(r, v.value.u16);
    }
    return true;
}

static bool
rds_u16_des(const RegisterAtom *r, RegisterValue *v, const bool bigendian)
{
    if (bigendian) {
        v->value.u16 = bf_ref_u16b(r);
    } else {
        v->value.u16 = bf_ref_u16l(r);
    }
    v->type = REG_TYPE_UINT16;
    return true;
}

static bool
rds_u32_ser(const RegisterValue v, RegisterAtom *r, const bool bigendian)
{
    assert(v.type == REG_TYPE_UINT32);
    if (bigendian) {
        bf_set_u32b(r, v.value.u32);
    } else {
        bf_set_u32l(r, v.value.u32);
    }
    return true;
}

static bool
rds_u32_des(const RegisterAtom *r, RegisterValue *v, const bool bigendian)
{
    if (bigendian) {
        v->value.u32 = bf_ref_u32b(r);
    } else {
        v->value.u32 = bf_ref_u32l(r);
    }
    v->type = REG_TYPE_UINT32;
    return true;
}

static bool
rds_u64_ser(const RegisterValue v, RegisterAtom *r, const bool bigendian)
{
    assert(v.type == REG_TYPE_UINT64);
    if (bigendian) {
        bf_set_u64b(r, v.value.u64);
    } else {
        bf_set_u64l(r, v.value.u64);
    }
    return true;
}

static bool
rds_u64_des(const RegisterAtom *r, RegisterValue *v, const bool bigendian)
{
    if (bigendian) {
        v->value.u64 = bf_ref_u64b(r);
    } else {
        v->value.u64 = bf_ref_u64l(r);
    }
    v->type = REG_TYPE_UINT64;
    return true;
}

static bool
rds_s16_ser(const RegisterValue v, RegisterAtom *r, const bool bigendian)
{
    assert(v.type == REG_TYPE_SINT16);
    if (bigendian) {
        bf_set_s16b(r, v.value.s16);
    } else {
        bf_set_s16l(r, v.value.s16);
    }
    return true;
}

static bool
rds_s16_des(const RegisterAtom *r, RegisterValue *v, const bool bigendian)
{
    if (bigendian) {
        v->value.s16 = bf_ref_s16b(r);
    } else {
        v->value.s16 = bf_ref_s16l(r);
    }
    v->type = REG_TYPE_SINT16;
    return true;
}

static bool
rds_s32_ser(const RegisterValue v, RegisterAtom *r, const bool bigendian)
{
    assert(v.type == REG_TYPE_SINT32);
    if (bigendian) {
        bf_set_s32b(r, v.value.s32);
    } else {
        bf_set_s32l(r, v.value.s32);
    }
    return true;
}

static bool
rds_s32_des(const RegisterAtom *r, RegisterValue *v, const bool bigendian)
{
    if (bigendian) {
        v->value.s32 = bf_ref_s32b(r);
    } else {
        v->value.s32 = bf_ref_s32l(r);
    }
    v->type = REG_TYPE_SINT32;
    return true;
}

static bool
rds_s64_ser(const RegisterValue v, RegisterAtom *r, const bool bigendian)
{
    assert(v.type == REG_TYPE_SINT64);
    if (bigendian) {
        bf_set_s64b(r, v.value.s64);
    } else {
        bf_set_s64l(r, v.value.s64);
    }
    return true;
}

static bool
rds_s64_des(const RegisterAtom *r, RegisterValue *v, const bool bigendian)
{
    if (bigendian) {
        v->value.s64 = bf_ref_s64b(r);
    } else {
        v->value.s64 = bf_ref_s64l(r);
    }
    v->type = REG_TYPE_SINT64;
    return true;
}

static bool
rds_f32_ser(const RegisterValue v, RegisterAtom *r, const bool bigendian)
{
    assert(v.type == REG_TYPE_FLOAT32);
    if ((v.value.f32 != 0.f) && (isnormal(v.value.f32) == false)) {
        return false;
    }
    if (bigendian) {
        bf_set_f32b(r, v.value.f32);
    } else {
        bf_set_f32l(r, v.value.f32);
    }
    return true;
}

static bool
rds_f32_des(const RegisterAtom *r, RegisterValue *v, const bool bigendian)
{
    if (bigendian) {
        v->value.f32 = bf_ref_f32b(r);
    } else {
        v->value.f32 = bf_ref_f32l(r);
    }
    v->type = REG_TYPE_FLOAT32;
    return ((v->value.f32 == 0.f) || (isnormal(v->value.f32) == true));
}

static bool
rds_f64_ser(const RegisterValue v, RegisterAtom *r, const bool bigendian)
{
    assert(v.type == REG_TYPE_FLOAT64);
    if ((v.value.f64 != 0.) && (isnormal(v.value.f64) == false)) {
        return false;
    }
    if (bigendian) {
        bf_set_f64b(r, v.value.f64);
    } else {
        bf_set_f64l(r, v.value.f64);
    }
    return true;
}

static bool
rds_f64_des(const RegisterAtom *r, RegisterValue *v, const bool bigendian)
{
    if (bigendian) {
        v->value.f64 = bf_ref_f64b(r);
    } else {
        v->value.f64 = bf_ref_f64l(r);
    }
    v->type = REG_TYPE_FLOAT64;
    return ((v->value.f64 == 0.) || (isnormal(v->value.f64) == true));
}

const RegisterSerDes rds_serdes[] = {
    [REG_TYPE_INVALID] = { rds_invalid_ser, rds_invalid_des },
    [REG_TYPE_UINT16]  = { rds_u16_ser,     rds_u16_des },
    [REG_TYPE_UINT32]  = { rds_u32_ser,     rds_u32_des },
    [REG_TYPE_UINT64]  = { rds_u64_ser,     rds_u64_des },
    [REG_TYPE_SINT16]  = { rds_s16_ser,     rds_s16_des },
    [REG_TYPE_SINT32]  = { rds_s32_ser,     rds_s32_des },
    [REG_TYPE_SINT64]  = { rds_s64_ser,     rds_s64_des },
    [REG_TYPE_FLOAT32] = { rds_f32_ser,     rds_f32_des },
    [REG_TYPE_FLOAT64] = { rds_f64_ser,     rds_f64_des }
};

#define rs(t) (sizeof(t) / sizeof(RegisterAtom))

const size_t rds_size[] = {
    [REG_TYPE_INVALID] = 0,
    [REG_TYPE_UINT16]  = rs(uint16_t), /* NOLINT */
    [REG_TYPE_UINT32]  = rs(uint32_t),
    [REG_TYPE_UINT64]  = rs(uint64_t),
    [REG_TYPE_SINT16]  = rs(int16_t),
    [REG_TYPE_SINT32]  = rs(int32_t),
    [REG_TYPE_SINT64]  = rs(int64_t),
    [REG_TYPE_FLOAT32] = rs(float),
    [REG_TYPE_FLOAT64] = rs(double),
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
    case REG_TYPE_FLOAT64:
        return (v.value.f64 >= limit.f64);
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
    case REG_TYPE_FLOAT64:
        return (v.value.f64 <= limit.f64);
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
rv_validate(RegisterTable *t, RegisterEntry *e, const RegisterValue v)
{
    if (e->type != v.type) {
        return false;
    }

    switch (e->check.type) {
    case REGV_TYPE_TRIVIAL:
        return true;
    case REGV_TYPE_FAIL:
        /* This allows for the default value to be applied at initialisation
         * time, but no further writes afterwards. */
        return BIT_ISSET(t->flags, REG_TF_DURING_INIT);
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

static inline int
reg_range_touches(RegisterEntry *e, RegisterAddress addr, RegisterOffset n)
{
    /* Return -1 if entry is below range; 0 if it is within the range and 1 if
     * it is above the range */
    const RegisterOffset size = rds_size[e->type];

    if ((e->address + size) <= addr) {
        return -1;
    }

    if ((addr + n) <= e->address) {
        return 1;
    }

    return 0;
}

static RegisterAccess
reg_entry_sane(RegisterTable *t, RegisterHandle reg)
{
    RegisterAccess rv;
    RegisterValue current;

    /* _get() runs the deserialiser; if it fails, this fails. */
    rv = register_get(t, reg, &current);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }

    /* Make sure the current value is fits into table constraints */
    if (rv_validate(t, t->entry + reg, current)) {
        rv.code = REG_ACCESS_SUCCESS;
        rv.address = 0u;
    } else {
        rv.code = REG_ACCESS_RANGE;
        rv.address = reg;
    }
    return rv;
}

static RegisterAccess
reg_entry_load_default(RegisterTable *t, RegisterHandle reg)
{
    RegisterEntry *e = t->entry + reg;
    const RegisterValue def = { .value = e->default_value,
                                .type = e->type };
    return register_set(t, reg, def);
}

static void
reg_taint_in_range(RegisterTable *t, RegisterAddress addr, RegisterOffset n)
{
    for (RegisterOffset i = 0ul; i < t->entries; ++i) {
        int touch = reg_range_touches(&t->entry[i], addr, n);
        if (touch > 0) {
            return;
        }
        if (touch < 0) {
            continue;
        }
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
    if (a->base > addr) {
        return false;
    }
    if ((a->base + a->size) <= addr) {
        return false;
    }
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
    const RegisterAddress entry_end = e->address + rds_size[e->type];
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
            if (ra_reg_fits_into(area, e) == false) {
                return false;
            }

            e->area = area;
            e->offset = e->address - area->base;
            return true;
        }
    }
    return false;
}

static inline RegisterAccess
reg_read_entry(RegisterEntry *e, RegisterAtom *buf)
{
    return e->area->read(e->area, buf, e->offset, rds_size[e->type]);
}

static inline int
ra_range_touches(RegisterArea *a, RegisterAddress addr, RegisterOffset n)
{
    /* Return -1 if area is below range; 0 if it is within the range and 1 if
     * it is above the range */
    if ((a->base + a->size) <= addr) {
        return -1;
    }
    if ((addr + n) <= a->base) {
        return 1;
    }
    return 0;
}
static RegisterHandle
ra_first_entry_of_next(RegisterTable *t, RegisterArea *a, RegisterHandle start)
{
    for (RegisterHandle i = start; i < t->entries; ++i) {
        if (ra_addr_is_part_of(a, t->entry[i].address) == false) {
            return i;
        }
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
        if (touch < 0) {
            continue;
        }
        if (touch > 0) {
            break;
        }
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
        const RegisterOffset size = rds_size[e->type];
        const RegisterAddress end = e->address + size - 1;
        RegisterAddress bs, rs;
        RegisterOffset rlen;

        /* Skip entries before block start */
        if (addr > end) {
            continue;
        }

        /* Terminate for entries after last */
        if (e->address > last) {
            break;
        }

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
        rv = reg_read_entry(e, raw);
        if (rv.code != REG_ACCESS_SUCCESS) {
            return rv;
        }
        memcpy(raw + rs, buf + bs, rlen * sizeof(RegisterAtom));

        /* Try the deserialiser, fail if it fails */
        const bool bigendian = BIT_ISSET(t->flags, REG_TF_BIG_ENDIAN);
        if (rds_serdes[e->type].des(raw, &datum, bigendian) == false) {
            rv.code = REG_ACCESS_INVALID;
            rv.address = addr + bs;
            return rv;
        }

        /* Try the validator, fail if it fails */
        if (rv_validate(t, e, datum) == false) {
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
        if (isxdigit((int)s[idx]) == false) {
            return false;
        }
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

static bool
need_to_load_default(const RegisterEntry *e)
{
    if (e->area->write == NULL) {
        return false;
    }

    if (BIT_ISSET(e->area->flags, REG_AF_SKIP_DEFAULTS)) {
        return false;
    }

    return true;
}

/* Public API */

size_t
register_entry_size(const RegisterEntry *e)
{
    return rds_size[e->type];
}

void
register_make_bigendian(RegisterTable *t, const bool bigendian)
{
    if (bigendian) {
        BIT_SET(t->flags, REG_TF_BIG_ENDIAN);
    } else {
        BIT_CLEAR(t->flags, REG_TF_BIG_ENDIAN);
    }
}

RegisterInit
register_init(RegisterTable *t) /* NOLINT */
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
    BIT_SET(t->flags, REG_TF_DURING_INIT);
    /* Determine table sizes first */
    t->areas = reg_count_areas(t->area);
    if (t->areas == AREA_HANDLE_MAX) {
        rv.code = REG_INIT_TOO_MANY_AREAS;
        rv.pos.area = AREA_HANDLE_MAX;
        BIT_CLEAR(t->flags, REG_TF_DURING_INIT);
        return rv;
    }
    t->entries = reg_count_entries(t->entry);
    if (t->entries == REGISTER_HANDLE_MAX) {
        rv.code = REG_INIT_TOO_MANY_ENTRIES;
        rv.pos.entry = REGISTER_HANDLE_MAX;
        BIT_CLEAR(t->flags, REG_TF_DURING_INIT);
        return rv;
    }

    if (t->areas == 0ul) {
        rv.code = REG_INIT_NO_AREAS;
        rv.pos.area = 0;
        BIT_CLEAR(t->flags, REG_TF_DURING_INIT);
        return rv;
    }

    previous = t->area[0].base;
    for (AreaHandle i = 1ul; i < t->areas; ++i) {
        const RegisterAddress current = t->area[i].base;
        if (current < previous) {
            rv.code = REG_INIT_AREA_INVALID_ORDER;
            rv.pos.area = i;
            BIT_CLEAR(t->flags, REG_TF_DURING_INIT);
            return rv;
        }
        if (current < (previous + t->area[i-1].size)) {
            rv.code = REG_INIT_AREA_ADDRESS_OVERLAP;
            rv.pos.area = i;
            BIT_CLEAR(t->flags, REG_TF_DURING_INIT);
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
            BIT_CLEAR(t->flags, REG_TF_DURING_INIT);
            return rv;
        }
        if (current < (previous+rds_size[t->entry[i-1].type])) {
            rv.code = REG_INIT_ENTRY_ADDRESS_OVERLAP;
            rv.pos.entry = i;
            BIT_CLEAR(t->flags, REG_TF_DURING_INIT);
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
        if (t->area[i].mem != NULL) {
            memset(t->area[i].mem, 0, t->area[i].size * sizeof(RegisterAtom));
        }
    }

    BIT_SET(t->flags, REG_TF_INITIALISED);
    for (RegisterHandle i = 0ul; i < t->entries; ++i) {
        RegisterEntry *e = &t->entry[i];
        /* Link into register table memory */
        bool success = reg_entry_is_in_memory(t, e);
        if (success == false) {
            rv.code = REG_INIT_ENTRY_IN_MEMORY_HOLE;
            rv.pos.entry = i;
            BIT_CLEAR(t->flags, REG_TF_INITIALISED | REG_TF_DURING_INIT);
            return rv;
        }
        e->area = &t->area[ra_find_area_by_addr(t, e->address)];

        if (need_to_load_default(e)) {
            RegisterAccess access;
            RegisterValue def;
            /* Load default value into register table */
            def.value = e->default_value;
            def.type = e->type;
            access = register_set(t, i, def);

            if (access.code != REG_ACCESS_SUCCESS) {
                rv.code = REG_INIT_ENTRY_INVALID_DEFAULT;
                rv.pos.entry = i;
                BIT_CLEAR(t->flags, REG_TF_INITIALISED | REG_TF_DURING_INIT);
                return rv;
            }
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

    BIT_CLEAR(t->flags, REG_TF_DURING_INIT);
    return rv;
}

/**
 * Run a function for each register entry for user initilisation
 *
 * Users may want to run their own initialisation on the contents of the user
 * pointers provided by the REGx_...() specifiers.
 *
 * This runs a registerCallback function with its table argument set to ‘t’,
 * the handle argument set to the iteration's handle and the custom void
 * pointer argument set to the iteration's registers's user pointer.
 *
 * When an iteration's function's return value is less than zero, iteration
 * stops and the return code is set to REG_ACCESS_FAILURE with the address
 * parameter set to the final iteration's register address.
 *
 * If the provided register table is not initialised by ‘register_init()’, the
 * function immediately returns setting the return code to
 * REG_ACCESS_UNINITIALISED.
 *
 * The iteration does not test for the user-pointer being non-NULL before
 * calling the function, as this may be one of the things users may want to
 * signal an error for.
 *
 * @param  t   Pointer to the register table to work on
 * @param  f   Function to call for each register entry.
 *
 * @return REG_ACCESS_UNINITIALISED if the table isn't initialised;
 *         REG_ACCESS_FAILURE if a callback returns a negative value.
 *         REG_ACCESS_SUCCESS otherwise.
 *
 * @sideeffects The iteration process itself is pure, but a callback function
 *              may introduce sideeffects.
 */
RegisterAccess
register_user_init(RegisterTable *t, registerCallback f)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;

    if (BIT_ISSET(t->flags, REG_TF_INITIALISED) == false) {
        rv.code = REG_ACCESS_UNINITIALISED;
        return rv;
    }

    for (RegisterHandle i = 0ul; i < t->entries; ++i) {
        const int code = f(t, i, t->entry[i].user);
        if (code < 0) {
            rv.code = REG_ACCESS_FAILURE;
            rv.address = t->entry[i].address;
            break;
        }
    }

    return rv;
}

static RegisterAccess
register_setx(RegisterTable *t, const RegisterHandle idx,
              const RegisterValue v, const bool withvalidator)
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

    e = t->entry + idx;

    if (withvalidator) {
        success = rv_validate(t, e, v);
        if (success == false) {
            rv.code = REG_ACCESS_RANGE;
            rv.address = e->address;
            return rv;
        }
    }

    a = e->area;

    if (register_area_can_write(a) == false) {
        rv.code = REG_ACCESS_READONLY;
        rv.address = e->address;
        return rv;
    }

    const bool bigendian = BIT_ISSET(t->flags, REG_TF_BIG_ENDIAN);
    success = rds_serdes[e->type].ser(v, raw, bigendian);

    if (success == false) {
        rv.code = REG_ACCESS_INVALID;
        rv.address = e->address;
        return rv;
    }

    return a->write(a, raw, e->offset, rds_size[e->type]);
}

RegisterAccess
register_set(RegisterTable *t, const RegisterHandle idx, const RegisterValue v)
{
    return register_setx(t, idx, v, true);
}

RegisterAccess
register_set_unsafe(RegisterTable *t, const RegisterHandle idx,
                    const RegisterValue v)
{
    return register_setx(t, idx, v, false);
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
    rv = a->read(a, raw, e->offset, rds_size[e->type]);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }
    const bool bigendian = BIT_ISSET(t->flags, REG_TF_BIG_ENDIAN);
    success = rds_serdes[e->type].des(raw, v, bigendian);

    if (success == false) {
        rv.code = REG_ACCESS_INVALID;
        rv.address = idx;
    }
    return rv;
}

/**
 * Set bits in a register
 *
 * This function reads a register, sets the bits indicated by the ‘v’ parameter
 * and writes the resulting value back to the register. The type of this ‘v’
 * parameter must match the one of the register addressed by ’idx’. Note that
 * bitwise manipulation is only supported on unsigned integer register types.
 *
 * REG_ACCESS_INVALID is returned either when the given register type does not
 * match to the one read from the register table or when the register type is
 * not supported, for example, float or signed integers.
 *
 * If the given address to the register is out of range, the function returns
 * immediately with the return code of REG_ACCESS_NOENTRY.
 *
 * If the provided register table is not initialised by ‘register_init()’, the
 * function immediately returns setting the return code to
 * REG_ACCESS_UNINITIALISED.
 *
 * @param  t     Pointer to the register table to work on
 * @param  idx   RegisterHandle (index) of the target register inside ‘t’
 * @param  v     Pointer to the register value where the target bit is set
 *
 * @return REG_ACCESS_INVALID       if the given type is mismatching or unsupported;
 *         REG_ACCESS_NOENTRY       if the given idx is out of range;
 *         REG_ACCESS_UNINITIALISED if the table isn't initialised;
 *         REG_ACCESS_SUCCESS otherwise.
 *
 * @sideeffects A bit in the register at the given idx is set to 1.
 */
RegisterAccess
register_bit_set(RegisterTable *t,
                 const RegisterHandle idx,
                 const RegisterValue v)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    RegisterValue reg;

    rv = register_get(t, idx, &reg);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }

    if (reg.type != v.type) {
        goto invalid;
    }

    switch (reg.type) {
    case REG_TYPE_UINT16:
        BIT_SET(reg.value.u16, v.value.u16);
        break;
    case REG_TYPE_UINT32:
        BIT_SET(reg.value.u32, v.value.u32);
        break;
    case REG_TYPE_UINT64:
        BIT_SET(reg.value.u64, v.value.u64);
        break;
    case REG_TYPE_SINT16:
        /* FALLTHROUGH */
    case REG_TYPE_SINT32:
        /* FALLTHROUGH */
    case REG_TYPE_SINT64:
        /* FALLTHROUGH */
    case REG_TYPE_FLOAT32:
        /* FALLTHROUGH */
    case REG_TYPE_FLOAT64:
        /* FALLTHROUGH */
    case REG_TYPE_INVALID:
        goto invalid;
    }
    rv = register_set(t, idx, reg);
    return rv;

invalid:
    rv.code = REG_ACCESS_INVALID;
    rv.address = idx;
    return rv;
}

/**
 * Clear bits in a register
 *
 * This function reads a register, clears the bits indicated by the ‘v’
 * parameter and writes the resulting value back to the register. The type of
 * this ‘v’ parameter must match the one of the register addressed by ’idx’.
 * Note that bitwise manipulation is only supported on unsigned integer
 * register types.
 *
 * REG_ACCESS_INVALID is returned either when the given register type does not
 * match to the one read from the register table or when the register type is
 * not supported, for example, float or signed integers.
 *
 * If the given address to the register is out of range, the function returns
 * immediately with the return code of REG_ACCESS_NOENTRY.
 *
 * If the provided register table is not initialised by ‘register_init()’, the
 * function immediately returns setting the return code to
 * REG_ACCESS_UNINITIALISED.
 *
 * @param  t     Pointer to the register table to work on
 * @param  idx   RegisterHandle (index) of the target register inside ‘t’
 * @param  v     Pointer to the register value where the target bit is set
 *
 * @return REG_ACCESS_INVALID       if the given type is mismatching or unsupported;
 *         REG_ACCESS_NOENTRY       if the given idx is out of range;
 *         REG_ACCESS_UNINITIALISED if the table isn't initialised;
 *         REG_ACCESS_SUCCESS otherwise.
 *
 * @sideeffects A bit in the register at the given idx is set to 0.
 */
RegisterAccess
register_bit_clear(RegisterTable *t,
                   const RegisterHandle idx,
                   const RegisterValue v)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    RegisterValue reg;

    rv = register_get(t, idx, &reg);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }

    if (reg.type != v.type) {
        goto invalid;
    }

    switch (reg.type) {
    case REG_TYPE_UINT16:
        BIT_CLEAR(reg.value.u16, v.value.u16);
        break;
    case REG_TYPE_UINT32:
        BIT_CLEAR(reg.value.u32, v.value.u32);
        break;
    case REG_TYPE_UINT64:
        BIT_CLEAR(reg.value.u64, v.value.u64);
        break;
    case REG_TYPE_SINT16:
        /* FALLTHROUGH */
    case REG_TYPE_SINT32:
        /* FALLTHROUGH */
    case REG_TYPE_SINT64:
        /* FALLTHROUGH */
    case REG_TYPE_FLOAT32:
        /* FALLTHROUGH */
    case REG_TYPE_FLOAT64:
        /* FALLTHROUGH */
    case REG_TYPE_INVALID:
        goto invalid;
    }
    rv = register_set(t, idx, reg);
    return rv;

invalid:
    rv.code = REG_ACCESS_INVALID;
    rv.address = idx;
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
RegisterAccess
register_block_read_unsafe(RegisterTable *t, RegisterAddress addr,
                           RegisterOffset n, RegisterAtom *buf)
{
    RegisterAccess rv;
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
            rv = a->read(a, buf, offset, readn);
            if (rv.code != REG_ACCESS_SUCCESS) {
                return rv;
            }
        } else {
            /* Memory that can't be read reads back zeroes */
            memset(buf + offset, 0, sizeof(RegisterAtom) * readn);
        }

        buf += readn;
        addr += readn;
        rest -= readn;
    }
    rv = (RegisterAccess)REG_ACCESS_RESULT_INIT;
    return rv;
}

RegisterAccess
register_block_write_unsafe(RegisterTable *t, RegisterAddress addr,
                            RegisterOffset n, RegisterAtom *buf)
{
    RegisterAccess rv;
    RegisterOffset rest = n;
    while (rest > 0ull) {
        AreaHandle an = ra_find_area_by_addr(t, addr);
        RegisterOffset offset, writen;
        RegisterArea *a;

        assert(an < t->areas);
        a = &t->area[an];
        offset = addr - a->base;
        writen = reg_min(a->base + a->size - addr, rest);
        rv = a->write(a, buf, offset, writen);
        if (rv.code != REG_ACCESS_SUCCESS) {
            return rv;
        }
        buf += writen;
        addr += writen;
        rest -= writen;
    }
    rv = (RegisterAccess)REG_ACCESS_RESULT_INIT;
    return rv;
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

    if (n == 0ull) {
        return rv;
    }

    rv = register_block_touches_hole(t, addr, n);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }

    return register_block_read_unsafe(t, addr, n, buf);
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
    if (n == 0ull) {
        return rv;
    }

    rv = ra_writeable(t, addr, n);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }

    /* Make sure the block write instruction does not want to write into
     * an address that does not map to an area in the register table. */

    rv = register_block_touches_hole(t, addr, n);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }

    rv = ra_malformed_write(t, addr, n, buf);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }

    /* If the previous validation steps succeeded, it is safe to push this
     * chunk of memory into the referenced register table. Since we checked for
     * all errors before hand, it is safe to use this function. */

    rv = register_block_write_unsafe(t, addr, n, buf);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }
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
        const size_t size = rds_size[REG_TYPE_UINT16];
        rv = area->write((RegisterArea*)area, &value, o, size);
        if (rv.code != REG_ACCESS_SUCCESS) {
            return rv;
        }
    }

    return rv;
}

/**
 * Transfer Data from one area into another
 *
 * This function requires one of the areas (either source or destination) to be
 * system memory backened. REG_ACCESS_INVALID is returned if neither area
 * satisfies this requirement.
 *
 * The function transfers either the size of the source area or the size of the
 * destination area, depending on which of the two contains the least amount of
 * data.
 *
 * @param  dst    Handle to destination area
 * @param  src    Handle to source area
 *
 * @return Error condition arising from copy process.
 * @sideeffects Source area data is transferred into destination area.
 */
RegisterAccess
register_mcopy(RegisterTable *t, AreaHandle dst, AreaHandle src)
{
    RegisterAccess rv = REG_ACCESS_RESULT_INIT;
    if ((t->area[dst].mem == NULL) && (t->area[src].mem == NULL)) {
        rv.code = REG_ACCESS_INVALID;
        return rv;
    }

    RegisterArea *da = &(t->area[dst]);
    RegisterArea *sa = &(t->area[src]);
    const size_t n = (da->size < sa->size) ? da->size : sa->size;

    if ((t->area[dst].mem != NULL) && (t->area[src].mem != NULL)) {
        /* Both areas are memory backed; just use memcpy */
        memcpy(da->mem, sa->mem, n * sizeof(RegisterAtom));
        return rv;
    } else if (t->area[dst].mem == NULL) {
        /* Destination buffer has to be accessed via its block-write API */
        return da->write(da, sa->mem, 0u, n);
    } else {
        /* Source buffer has to be accessed via its block-read API */
        return sa->read(sa, da->mem, 0u, n);
    }
}

bool
register_value_compare(const RegisterValue *a, const RegisterValue *b)
{
    if (a->type != b->type) {
        return false;
    }

    switch (a->type) {
    case REG_TYPE_UINT16:
        return (a->value.u16 == b->value.u16);
    case REG_TYPE_UINT32:
        return (a->value.u32 == b->value.u32);
    case REG_TYPE_UINT64:
        return (a->value.u64 == b->value.u64);
    case REG_TYPE_SINT16:
        return (a->value.s16 == b->value.s16);
    case REG_TYPE_SINT32:
        return (a->value.s32 == b->value.s32);
    case REG_TYPE_SINT64:
        return (a->value.s64 == b->value.s64);
    case REG_TYPE_FLOAT32:
        return (a->value.f32 == b->value.f32);
    case REG_TYPE_FLOAT64:
        return (a->value.f64 == b->value.f64);
    default:
        return false;
    }
}

RegisterAccess
register_compare(RegisterTable *t, RegisterHandle a, RegisterHandle b)
{
    RegisterAccess rv;
    RegisterValue av, bv;

    rv = register_get(t, a, &av);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }
    rv = register_get(t, b, &bv);
    if (rv.code != REG_ACCESS_SUCCESS) {
        return rv;
    }

    if (register_value_compare(&av, &bv)) {
        rv.code = REG_ACCESS_SUCCESS;
    } else {
        rv.code = REG_ACCESS_FAILURE;
    }
    return rv;
}

RegisterAccess
register_sanitise(RegisterTable *t)
{
    RegisterAccess rv = { .code = REG_ACCESS_SUCCESS, .address = 0u };

    if (BIT_ISSET(t->flags, REG_TF_INITIALISED) == false) {
        rv.code = REG_ACCESS_UNINITIALISED;
        return rv;
    }

    for (RegisterOffset i = 0ul; i < t->entries; ++i) {
        RegisterAccess access;
        access = reg_entry_sane(t, i);
        switch (access.code) {
        case REG_ACCESS_SUCCESS:
            break;
        case REG_ACCESS_INVALID: /* FALLTHROUGH */
        case REG_ACCESS_RANGE:
            access = reg_entry_load_default(t, i);
            if (access.code != REG_ACCESS_SUCCESS) {
                return access;
            }
            break;
        default:
            /* Everything shouldn't ever happen */
            return access;
        }

        register_untouch(t, i);
    }

    return rv;
}

/* Linear search: Simple and likely sufficient with short register tables. We
 * can replace with with bisection if this turns out not to be the case. */

static struct maybe_area
find_area(const RegisterTable *t,
          AreaHandle first, AreaHandle last,
          RegisterAddress addr)
{
    struct maybe_area rv = { .valid = true, .handle = 0 };

    for (AreaHandle i = first; i <= last; i++) {
        if (ra_addr_is_part_of(t->area + i, addr)) {
            rv.handle = i;
            return rv;
        }
    }

    rv.valid = false;
    return rv;
}

static struct maybe_register
find_reg(const RegisterTable *t,
         RegisterHandle first, RegisterHandle last,
         RegisterAddress addr)
{
    struct maybe_register rv = { .valid = true, .handle = 0 };

    for (RegisterHandle i = first; i <= last; i++) {
        if (reg_range_touches(t->entry + i, addr, 1u) == 0) {
            rv.handle = i;
            return rv;
        }
    }

    rv.valid = false;
    return rv;
}

static RegisterAccess
reg_iterate(RegisterTable *t,
            RegisterHandle start, RegisterAddress end,
            registerCallback f, void *arg)
{
    RegisterAccess rv = { .code = REG_ACCESS_SUCCESS, .address = 0u };
    RegisterHandle last = t->entries - 1u;

    while (start <= last && t->entry[start].address <= end) {
        int iret = f(t, start, arg);

        if (LIKELY(iret == 0)) {
            start++;
            continue;
        } else if (iret < 0) {
            rv.code = REG_ACCESS_FAILURE;
            rv.address = t->entry[start].address;
            return rv;
        } else {
            return rv;
        }
    }

    return rv;
}

/**
 * Call a function for each register defined within a range of addresses
 *
 * This function is lenient towards issues like memory holes. Which means, that
 * you can easily iterate over all the registers in a register table by doing:
 *
 * @code
 * register_foreach_in(t, REGISTER_ADDRESS_MAX,  REGISTER_OFFSET_MAX, f, NULL);
 * @endcode
 *
 * The iteration process stops whenever an iteration function returns a
 * non-zero value. A negative value will stop the iteration signaling failure.
 * A positive value will stop the iteration process signaling success.
 *
 * @param  t       The register table to work within
 * @param  addr    Start of the address range to work in
 * @param  off     Length of the range to work in
 * @param  f       Callback function to call on registers
 * @param  arg     Additional argument to pass to f
 *
 * @return REG_ACCESS_UNINITIALISED if the table isn't initialised;
 *         REG_ACCESS_FAILURE if a callback returns a negative value; the
 *         address field of the return value is set to the address of the entry
 *         at this point; REG_ACCESS_SUCCESS otherwise.
 *
 * @sideeffects The iteration process itself is pure, but a callback function
 *              may introduce sideeffects
 */
RegisterAccess
register_foreach_in(RegisterTable *t,
                    RegisterAddress addr, RegisterOffset off,
                    registerCallback f, void *arg)
{
    RegisterAccess rv = { .code = REG_ACCESS_SUCCESS, .address = 0u };

    if (BIT_ISSET(t->flags, REG_TF_INITIALISED) == false) {
        /* Can't do anything with a table that's not initialised. */
        rv.code = REG_ACCESS_UNINITIALISED;
        return rv;
    } else if (off == 0u || t->entries == 0u) {
        /* If the table has no entries, we're done with no work. */
        return rv;
    }

    /*
     * Find the first register in the given range.
     *
     * If addr is mapped, first looking up by area, then by entry within that
     * area works with the least amount of operations. If addr is not mapped,
     * we're performing the look-up over all entries within the table.
     */
    struct maybe_area startarea = find_area(t, 0, t->areas - 1u, addr);
    struct maybe_register startreg;

    if (startarea.valid) {
        const RegisterHandle first = t->area[startarea.handle].entry.first;
        const RegisterHandle last = t->area[startarea.handle].entry.last;
        startreg = find_reg(t, first, last, addr);
    } else {
        startreg = find_reg(t, 0, t->entries - 1u, addr);
    }

    if (startreg.valid == false) {
        return rv;
    }

    return reg_iterate(t, startreg.handle, addr + off - 1u, f, arg);
}

RegisterEntry *
register_get_entry(const RegisterTable *t, const RegisterHandle r)
{
    if (r >= t->entries) {
        return NULL;
    }

    return t->entry + r;
}
