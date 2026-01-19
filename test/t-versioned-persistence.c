/*
 * Copyright (c) 2025-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <limits.h>
#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/test/tap.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>
#include <ufw/crc/crc16-arc.h>
#include <ufw/endpoints.h>
#include <ufw/versioned-persistence.h>

/*
 * Test setup
 */

/*#define TEST_DEBUG*/
#define STORAGE_SIZE 0x200

struct info_data {
    uint32_t a;
    uint32_t b;
    uint16_t c;
    uint16_t d;
    uint32_t e;
    uint16_t f;
    uint32_t g;
    uint32_t h;
    uint16_t i;
    uint32_t j;
};

#define INFO_DATA_DEFAULT { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }
#define INFO_SIZE    (sizeof(struct info_data))
#define INFO_ADDRESS 24U
#define INFO_VERSION  5U

static unsigned char storage[STORAGE_SIZE];
static unsigned char info[INFO_SIZE];

static void
t_reset(VersionedPersistence *vp)
{
    memset(storage, (int)UCHAR_MAX, STORAGE_SIZE);
#ifdef TEST_DEBUG
    printf("# vp_init()\n");
#endif /* TEST_DEBUG */
    const int initrc = vp_init(vp);
    unless (ok(initrc == 0, "VersionedPersistence initialisation works")) {
        printf("# ERROR: init: %d %s\n", -initrc, strerror(-initrc));
    }
#ifdef TEST_DEBUG
    printf("# vp_invalidate()\n");
#endif /* TEST_DEBUG */
    const int invalrc = vp_invalidate(vp, VP_DATA_META | VP_DATA_PAYLOAD);
    unless (ok(invalrc == 0, "VersionedPersistence invalidation works")) {
        printf("# ERROR: invalidate: %d %s\n", -initrc, strerror(-initrc));
    }
#ifdef TEST_DEBUG
    thexdump(storage, STORAGE_SIZE);
#endif /* TEST_DEBUG */
}

/*
 * Simulating a peripheral API
 */

static ssize_t
buffer_read(void *data, void *buf, size_t n)
{
    size_t *addr = (size_t*)data;
#ifdef TEST_DEBUG
    printf("# read(0x%08zx, buf, %zu)\n", *addr, n);
#endif /* TEST_DEBUG */
    if ((*addr + n) > STORAGE_SIZE) {
        return -ENOBUFS;
    }

    if (n > 6) {
        n = 6;
    }

    memcpy(buf, storage + *addr, n);
#ifdef TEST_DEBUG
    thexdump(buf, n);
#endif /* TEST_DEBUG */
    *addr += n;
    return n;
}

static ssize_t
buffer_write(void *data, const void *buf, size_t n)
{
    size_t *addr = (size_t*)data;
#ifdef TEST_DEBUG
    printf("# write(0x%08zx, buf, %zu)\n", *addr, n);
#endif /* TEST_DEBUG */
    if ((*addr + n) > STORAGE_SIZE) {
        return -ENOBUFS;
    }

    if (n > 5) {
        n = 5;
    }

    memcpy(storage + *addr, buf, n);
#ifdef TEST_DEBUG
    thexdump(storage + *addr, n);
#endif /* TEST_DEBUG */
    *addr += n;
    return n;
}

static int
seek_ep(void *data, const size_t n)
{
    *((size_t*)data) = n;
    return 0;
}

/* Tests */

#ifdef TEST_DEBUG
#define oprint(member)                                                  \
    do {                                                                \
        e = s + o;                                                      \
        o = offsetof(struct info_data, member);                         \
        s = sizeof(id. member);                                         \
        p = o - e;                                                      \
        printf("#   uint32_t %s; ...s: %2zu, o: %2zu, p = %2zu%s\n",    \
               #member, s, o, p, (p > 0) ? "  **padded**" : "");        \
    } while (0)
#endif /* TEST_DEBUG */

int
main(UNUSED int argc, UNUSED char *argv[])
{
    size_t source_offset = 0;
    size_t sink_offset = 0;

    Source storage_fetch = CHUNK_SOURCE_INIT(buffer_read, &source_offset);
    storage_fetch.ext.seek = seek_ep;
    Sink storage_store = CHUNK_SINK_INIT(buffer_write, &sink_offset);
    storage_store.ext.seek = seek_ep;

    VersionedPersistence vp =
        VP_SIMPLE_INIT(INFO_ADDRESS, INFO_SIZE, INFO_VERSION,
                       storage_fetch, storage_store);

#ifdef TEST_DEBUG
    {
        printf("# struct info_data (%zu, 0x%02zx):\n",
               sizeof(struct info_data),
               sizeof(struct info_data));

        struct info_data id;
        size_t s, o, e, p;

        o = offsetof(struct info_data, a);
        s = sizeof(id.a);
        printf("#   uint32_t a; s: %2zu, o: %2zu\n", s, o);
        oprint(b);
        oprint(c);
        oprint(d);
        oprint(e);
        oprint(f);
        oprint(g);
        oprint(h);
        oprint(i);
        oprint(j);
        printf("#\n");
    }
#endif /* TEST_DEBUG */

    plan(41);

    t_reset(&vp);

    {
        const int openrc = vp_refresh(&vp);
        unless (ok(-openrc == EBADFD,
                   "After t_reset, vp cannot be opened: Meta data error."))
        {
            prs16(-openrc, EBADFD);
        }
        unless (ok(vp.state == 0, "After t_reset, vp state is empty")) {
            prs16(vp.state, 0);
        }
    }

    {
        unsigned char *storage_info =
            storage
            + VP_SIZE_META
            + INFO_ADDRESS;
        unsigned char readback[4];
        unsigned char expect[] = { 0, 0, 255, 255 };
        unsigned char *border =
            storage_info
            + INFO_SIZE
            - (sizeof(readback)/2);

        memset(info, 0xffU, INFO_SIZE);
        cmp_mem(info, storage_info, INFO_SIZE,
                "Before format, info section is all 0xff");

        const int formatrc = vp_format(&vp, 0);
        unless (ok(formatrc == 0, "vp_format returns success")) {
            prs16(formatrc, 0);
        }
        memcpy(readback, border, sizeof(readback));
        cmp_mem(expect, readback, sizeof(readback),
                "Format does not extend beyound section");

        memset(info, 0, INFO_SIZE);
        cmp_mem(info, storage_info, INFO_SIZE,
                "After format, info section is all 0x00");
    }

    {
        struct info_data infodef = INFO_DATA_DEFAULT;

        const int openrc = vp_refresh(&vp);
        unless (ok(openrc == 0, "After t_format, vp can be opened")) {
            prs16(openrc, 0);
        }

        ssize_t storerc = vp_store(&vp, &infodef);
        unless (ok(storerc == sizeof(infodef),
                   "vp_store succeeded (default)"))
        {
            prs64(storerc, sizeof(infodef));
        }

        infodef.e = 0x12345678UL;
        storerc = vp_store(&vp, &infodef);
        unless (ok(storerc == sizeof(infodef),
                   "vp_store succeeded (e changed)"))
        {
            prs64(storerc, sizeof(infodef));
        }

        struct info_data load;
        const ssize_t fetchrc = vp_fetch(&vp, &load);
        unless (ok(fetchrc == sizeof(load), "vp_fetch succeeded")) {
            prs64(storerc, sizeof(load));
        }

        cmp_mem(&infodef, &load, sizeof(struct info_data),
                "Loading data yields same contents");
    }

    {
        /* Flip a bit in payload and see things fail */
        const unsigned char mask = BIT(5);
        const size_t pos =
            INFO_ADDRESS + VP_SIZE_META + INFO_SIZE
            - (INFO_SIZE / 3);

        storage[pos] ^= mask;

        const int rc0 = vp_refresh(&vp);
        unless (ok(rc0 == -EBADMSG, "After flipping a bit payload is invalid"))
        {
            prs16(-rc0, EBADMSG);
        }

        /* State should show everything as good, except the consistency of the
         * payload portion. */
        const uint16_t expstate0 =
            VP_STATE_VERSION_COMPATIBLE |
            VP_STATE_LENGTH_COMPATIBLE  |
            VP_STATE_PAYLOAD_COMPATIBLE |
            VP_STATE_META_CONSISTENT;
        cmp_mem(&vp.state, &expstate0, sizeof(vp.state),
                "state is as expected (0x%04"PRIx16")", expstate0);

        storage[pos] ^= mask;
        const int rc1 = vp_refresh(&vp);
        unless (ok(rc1 == 0, "After flipping it back payload is valid"))
        {
            prs16(rc0, 0);
        }

        /* Here everything should be good */
        const uint16_t expstate1 =
            VP_STATE_VERSION_COMPATIBLE |
            VP_STATE_LENGTH_COMPATIBLE  |
            VP_STATE_PAYLOAD_COMPATIBLE |
            VP_STATE_PAYLOAD_CONSISTENT |
            VP_STATE_META_CONSISTENT;
        cmp_mem(&vp.state, &expstate1, sizeof(vp.state),
                "state is as expected (0x%04"PRIx16")", expstate1);
    }

    {
        /* Flip a bit in payload checksum, which should invalidate the meta
         * block, which will invalidate everything. */
        const unsigned char mask = BIT(3);
        const size_t pos = INFO_ADDRESS + VP_OFFSET_PAYLOAD_CHKSUM - 1U;

        storage[pos] ^= mask;

        const int rc0 = vp_refresh(&vp);
        unless (ok(rc0 == -EBADFD, "Breaking a header bit breaks everything"))
        {
            prs16(-rc0, EBADFD);
        }

        const uint16_t expstate0 = 0U;

        cmp_mem(&vp.state, &expstate0, sizeof(vp.state),
                "state is as expected (0x%04"PRIx16")", expstate0);

        storage[pos] ^= mask;
        const int rc1 = vp_refresh(&vp);
        unless (ok(rc1 == 0, "After flipping it back payload is valid"))
        {
            prs16(rc0, 0);
        }

        /* Here everything should be good */
        const uint16_t expstate1 =
            VP_STATE_VERSION_COMPATIBLE |
            VP_STATE_LENGTH_COMPATIBLE  |
            VP_STATE_PAYLOAD_COMPATIBLE |
            VP_STATE_PAYLOAD_CONSISTENT |
            VP_STATE_META_CONSISTENT;
        cmp_mem(&vp.state, &expstate1, sizeof(vp.state),
                "state is as expected (0x%04"PRIx16")", expstate1);
    }

    {
        vp_put_version(&vp, INFO_VERSION - 1U);
        const int updaterc = vp_update_meta(&vp);
        ok(updaterc == 0U, "vp_update_meta() succeeded");

        const int refreshrc = vp_refresh(&vp);
        ok(refreshrc == 0U, "vp_refresh() succeeded");

        const bool usable = vp_usable(&vp);
        ok(usable == false,
           "Payload not usable because of version mismatch");

        const uint16_t expstate =
            VP_STATE_LENGTH_COMPATIBLE  |
            VP_STATE_PAYLOAD_CONSISTENT |
            VP_STATE_META_CONSISTENT;
        cmp_mem(&vp.state, &expstate, sizeof(vp.state),
                "state is as expected (0x%04"PRIx16")", expstate);

        /* Re-establishing previous state */
        vp_put_version(&vp, INFO_VERSION);
        vp_update_meta(&vp);
    }

    {
        vp_put_length(&vp, INFO_SIZE + 10);
        const int updaterc = vp_update_meta(&vp);
        ok(updaterc == 0U, "vp_update_meta() succeeded");

        const int refreshrc = vp_refresh(&vp);
        ok(refreshrc == 0U, "vp_refresh() succeeded");

        const bool usable = vp_usable(&vp);
        ok(usable == false,
           "Payload not usable because of size mismatch");

        const uint16_t expstate =
            VP_STATE_VERSION_COMPATIBLE |
            VP_STATE_PAYLOAD_CONSISTENT |
            VP_STATE_META_CONSISTENT;
        cmp_mem(&vp.state, &expstate, sizeof(vp.state),
                "state is as expected (0x%04"PRIx16")", expstate);

        /* Re-establishing previous state */
        vp_put_length(&vp, INFO_SIZE);
        vp_update_meta(&vp);
    }

    {
        const size_t specsize = vp_spec_size(&vp);
        const size_t sectsize = vp_section_size(&vp);
        const size_t macsize  = VP_SECTION_SIZE(INFO_SIZE);

        okx(specsize == sectsize);
        okx(specsize == macsize);
        okx(sectsize == macsize);
    }

    okx(vp_memset(&vp, 0, 0U, INFO_SIZE + 1) == -EINVAL);
    okx(vp_invalidate(&vp, 0) == 0);

    t_reset(&vp);


    {
        unsigned char dummy[0x200];
        ok(vp_fetch_part(&vp, &dummy, 0, INFO_SIZE) == -EINVAL,
           "Fetching does not work with inconstent header");
        ok(vp_store_part(&vp, &dummy, 0, INFO_SIZE) == -EINVAL,
           "Storing does not work with inconstent header");

        struct info_data infodef = INFO_DATA_DEFAULT;
        okx(vp_save(&vp, &infodef));

        ok(vp_fetch_part(&vp, &dummy, 0, INFO_SIZE + 1) == -EFAULT,
           "Fetching out of bounds does not work");
        ok(vp_store_part(&vp, &dummy, 0, INFO_SIZE + 1) == -EFAULT,
           "Storing out of bounds does not work");
    }

#ifdef TEST_DEBUG
    thexdump(storage, STORAGE_SIZE);
#endif /* TEST_DEBUG */

    return EXIT_SUCCESS;
}
