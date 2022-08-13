#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/test/tap.h>

#include <ufw/persistent-storage.h>

#define BUFFER_SIZE 128u
unsigned char buffer[BUFFER_SIZE];

static size_t
buffer_read(void *buf, uint32_t addr, size_t n)
{
    if ((addr + n) > BUFFER_SIZE)
        return 0;

    memcpy(buf, buffer + addr, n);
    return n;
}

static size_t
buffer_write(uint32_t addr, const void *buf, size_t n)
{
    if ((addr + n) > BUFFER_SIZE)
        return 0;

    memcpy(buffer + addr, buf, n);
    return n;
}

struct cfg {
    long double a;
    double b;
    uint32_t c;
};

static struct cfg set1, set2;

static void
t_init(void)
{
    set1.a = 1234.32;
    set1.b = 4321.23;
    set1.c = UINT32_MAX;
    memset(buffer, 0, BUFFER_SIZE);
}

static void
t_simple_store(unsigned char *b, size_t n, bool corrupt)
{
    PersistentAccess success;
    PersistentStorage store;

    t_init();

    persistent_init(&store, sizeof(struct cfg), buffer_read, buffer_write);

    if (b != NULL)
        persistent_buffer(&store, b, n);

    success = persistent_store(&store, &set1);
    unless (ok(success == PERSISTENT_ACCESS_SUCCESS, "set1 was stored")) {
        pru16(success, PERSISTENT_ACCESS_SUCCESS);
    }

    if (corrupt) {
        for (size_t i = 0u; i < BUFFER_SIZE; ++i) {
            if (buffer[i] != 0u) {
                buffer[i] = 0u;
                break;
            }
        }
        success = persistent_validate(&store);
        unless (ok(success == PERSISTENT_ACCESS_INVALID_DATA,
                   "Stored data failed to validate")) {
            pru16(success, PERSISTENT_ACCESS_INVALID_DATA);
        }
        return;
    }

    success = persistent_validate(&store);
    unless (ok(success == PERSISTENT_ACCESS_SUCCESS, "Stored data validated")) {
        pru16(success, PERSISTENT_ACCESS_SUCCESS);
    }

    success = persistent_fetch(&set2, &store);
    unless (ok(success == PERSISTENT_ACCESS_SUCCESS, "set2 was fetched")) {
        pru16(success, PERSISTENT_ACCESS_SUCCESS);
    }

    cmp_mem(&set1, &set2, sizeof(struct cfg),
            "fetch put contents of set1 into set2");

    long double foo;
    success = persistent_fetch_part(&foo, &store,
                                    offsetof(struct cfg, a),
                                    sizeof(foo));

    unless (ok(success == PERSISTENT_ACCESS_SUCCESS,
               "part foo was fetched successfully")) {
        pru16(success, PERSISTENT_ACCESS_SUCCESS);
    }
    ok(foo == set1.a, "foo has correct value");
}

int
main(UNUSED int argc, UNUSED char *argv[])
{
    unsigned char b[8u];
    plan(6 + 6 + 2 + 2);

    t_simple_store(NULL, 0u, false);                   /* 6 */
    t_simple_store(b, sizeof(b) / sizeof(*b), false);  /* 6 */
    t_simple_store(NULL, 0u, true);                    /* 2 */
    t_simple_store(b, sizeof(b) / sizeof(*b), true);   /* 2 */

    return EXIT_SUCCESS;
}
