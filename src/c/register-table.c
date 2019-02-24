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

#define regmem_read(rm, dst, o, n) ((rm).read(&(rm), (dst), (o), (n)))
#define regmem_write(rm, src, o, n) ((rm).write(&(rm), (src), (o), (n)))

static bool
rds_invalid_ser(const RegisterValue *v, RegisterAtom *r)
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
rds_u16_ser(const RegisterValue *v, RegisterAtom *r)
{
    assert(v->type == REG_TYPE_UINT16);
    *r = v->value.u16;
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
rds_u32_ser(const RegisterValue *v, RegisterAtom *r)
{
    assert(v->type == REG_TYPE_UINT32);
    *r = v->value.u32 & 0xfffful;
    *(r+1) = (v->value.u32 >> 16u) & 0xfffful;
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
rds_u64_ser(const RegisterValue *v, RegisterAtom *r)
{
    assert(v->type == REG_TYPE_UINT64);
    *(r+0) = (v->value.u64 >>  0u) & 0xfffful;
    *(r+1) = (v->value.u64 >> 16u) & 0xfffful;
    *(r+2) = (v->value.u64 >> 32u) & 0xfffful;
    *(r+3) = (v->value.u64 >> 48u) & 0xfffful;
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

#define rs(t) (sizeof(t) / sizeof(RegisterAtom))

const RegisterSerDes rds_serdes[] = {
    [REG_TYPE_INVALID] = { rds_invalid_ser, rds_invalid_des, 0 },
    [REG_TYPE_UINT16] = { rds_u16_ser, rds_u16_des, rs(uint16_t) },
    [REG_TYPE_UINT32] = { rds_u32_ser, rds_u32_des, rs(uint32_t) },
    [REG_TYPE_UINT64] = { rds_u64_ser, rds_u64_des, rs(uint64_t) }
};

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
ra_reg_is_part_of(RegisterArea *a, RegisterEntry *e)
{
    if (a->base > e->address)
        return false;

    if ((a->base + a->size) <= e->address)
        return false;

    return true;
}

static bool
ra_reg_fits_into(RegisterArea *a, RegisterEntry *e)
{
    const size_t area_end = a->base + a->size;
    const size_t entry_end = e->address + rds_serdes[e->type].size;
    return (area_end <= entry_end);
}

static bool
ra_find_memory(RegisterTable *t, RegisterEntry *e)
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
        bool success = ra_find_memory(t, e);
        if (success == false) {
            rv.code = REG_ACCESS_NOENTRY;
            rv.address = e->address;
            return rv;
        }
        /* Load default value into register table */
        def.value = e->default_value;
        def.type = e->type;
        success = rds_serdes[e->type].ser(&def, raw);

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
register_set(RegisterTable *t, size_t idx, RegisterValue *v)
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
