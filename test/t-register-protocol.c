/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/toolchain.h>

#include <ufw/binary-format.h>
#include <ufw/endpoints.h>
#include <ufw/register-protocol.h>

#include <ufw/test/tap.h>

/*
 * Build communication channel
 *
 * We're building our wire from memory, to be able to inspect it. Here we
 * really only need to provide the memory. The actual channel will be build
 * from endpoints (source and sink) connected to these buffers inside of
 * main().
 */

#define WIRE_SIZE 2048u

static unsigned char wire_local_to_remote[WIRE_SIZE];
static unsigned char wire_remote_to_local[WIRE_SIZE];

/*
 * The register-protocol provides access to a piece of memory (8 bit or 16 bit
 * wide atoms), and allows a remote side to read and write pieced of it. A fea-
 * tureful implementation of such a memory is in ufw's register-table module.
 *
 * To test the protocol module however, we're implementing the simplest possi-
 * ble memory that is interfaced by memcpy().
 */

#define MEMORY_SIZE 1024u

#define MAKE_MEMORY(TYPE,SIZE,SUFFIX)                           \
    static TYPE memory ## SUFFIX[SIZE];                         \
                                                                \
    static RPBlockAccess                                        \
    mread ## SUFFIX(uint32_t address,                           \
                    size_t bsize,                               \
                    TYPE *buf)                                  \
    {                                                           \
        RPBlockAccess rv = RPB_BLOCK_ACCESS_INIT;               \
                                                                \
        if (address + bsize > (SIZE)) {                         \
            rv.status = RP_RESP_EUNMAPPED;                      \
            rv.address = (SIZE) + 1u;                           \
            return rv;                                          \
        }                                                       \
                                                                \
        memcpy(buf, memory ## SUFFIX + address,                 \
               bsize * sizeof(*buf));                           \
                                                                \
        return rv;                                              \
    }                                                           \
                                                                \
    static RPBlockAccess                                        \
    mwrite ## SUFFIX(uint32_t address,                          \
                     size_t bsize,                              \
                     const TYPE *buf)                           \
    {                                                           \
        RPBlockAccess rv = RPB_BLOCK_ACCESS_INIT;               \
                                                                \
        if (address + bsize > (SIZE)) {                         \
            rv.status = RP_RESP_EUNMAPPED;                      \
            rv.address = (SIZE) + 1u;                           \
            return rv;                                          \
        }                                                       \
                                                                \
        memcpy(memory ## SUFFIX + address,                      \
               buf, bsize * sizeof(*buf));                      \
                                                                \
        return rv;                                              \
    }                                                           \

MAKE_MEMORY(uint16_t, MEMORY_SIZE, 16)

#ifdef WITH_UINT8_T
MAKE_MEMORY(uint8_t, MEMORY_SIZE, 8)
#endif /* WITH_UINT8_T */

/*
 * Some short-hands to specifies common tests quickly.
 */

#define okrc(str) unless (ok(rc == 0, (str))) {                 \
        printf("# rc: %d (%s)\n", rc, strerror(-rc));           \
    }

#define okmf(str) unless (ok(mf.error.id == 0,                          \
                             "%s: Frame reception okay", (str))) {      \
        printf("# id: %d (%s)\n", mf.error.id, strerror(mf.error.id));  \
        printf("# (maybe) framesize: %zu\n", mf.error.framesize);       \
    }

#define with_good_mf(MF) if ((MF).error.id == 0)

#ifdef WITH_UINT8_T
#define USE_CHECK_WIRE 1
static void
t_check_wire(const InstrumentableBuffer *wire,
             const void *expect, const size_t n,
             const size_t offset)
{
    const size_t required = n + offset;
    const size_t m = byte_buffer_rest(&wire->buffer);
    if (ok(required <= m,
           "channel: Expected bytes are available in wire (%zu <= %zu)",
           required, m))
    {
        const void *start = wire->buffer.data + wire->buffer.offset + offset;
        cmp_mem(start, expect, n, "channel: Data looks as expected o--~~--o");
    }
}
#endif /* WITH_UINT8_T */

/*
 * Test State
 */

/* Remote-to-Local machinery */
static InstrumentableBuffer r2l_buffer;
static Source r2l_source;
static Sink r2l_sink;
/* Local-to-Remote machinery */
static InstrumentableBuffer l2r_buffer;
static Source l2r_source;
static Sink l2r_sink;
/* Protocol Instances for local and remote */
static RegP p_local;
static RegP p_remote;
static RegP *local  = &p_local;
static RegP *remote = &p_remote;

static void
t_setup(const bool trace, const RPMemoryType mtype,
        const RPEndpointType eptype)
{
    for (size_t i = 0u; i < MEMORY_SIZE; ++i) {
        bf_set_u16l(memory16 + i, (uint16_t)(i & 0xffffu));
    }

#ifdef WITH_UINT8_T
    for (size_t i = 0u; i < MEMORY_SIZE; ++i) {
        memory8[i] = (uint8_t)(i & 0xffu);
    }
#endif /* WITH_UINT8_T */

    memset(wire_remote_to_local, 0xff, WIRE_SIZE);
    memset(wire_local_to_remote, 0xff, WIRE_SIZE);
    byte_buffer_rewind(&r2l_buffer.buffer);
    byte_buffer_rewind(&l2r_buffer.buffer);
    instrumentable_set_trace(&r2l_buffer, trace);
    instrumentable_set_trace(&l2r_buffer, trace);

    regp_reset_session(local);
    regp_reset_session(remote);

    if (mtype == RP_MEMTYPE_16) {
        regp_use_memory16(local, mread16, mwrite16);
    } else {
#ifdef WITH_UINT8_T
        regp_use_memory8(local, mread8, mwrite8);
#endif /* WITH_UINT8_T */
    }

    remote->ep.type = eptype;
    local->ep.type = eptype;
}

int
/* NOLINTNEXTLINE(readability-function-cognitive-complexity) */
main(UNUSED int argc, UNUSED char *argv[])
{
    regp_init(local);
    regp_init(remote);

    /* Initialise wire buffers */
    byte_buffer_space(&r2l_buffer.buffer,
                       wire_remote_to_local,
                       WIRE_SIZE);
    byte_buffer_space(&l2r_buffer.buffer,
                       wire_local_to_remote,
                       WIRE_SIZE);


    /* Initialise instrumentable wire endpoints */
    instrumentable_source(DATA_KIND_OCTET, &l2r_source, &l2r_buffer);
    instrumentable_sink(  DATA_KIND_OCTET, &l2r_sink,   &l2r_buffer);
    instrumentable_source(DATA_KIND_OCTET, &r2l_source, &r2l_buffer);
    instrumentable_sink(  DATA_KIND_OCTET, &r2l_sink,   &r2l_buffer);

    /* Initialise test state */
    t_setup(false, RP_MEMTYPE_16, RP_EP_TCP);

    /* Initialise register-protocol instances */
    regp_use_channel(local,  RP_EP_TCP, r2l_source, l2r_sink);
    regp_use_channel(remote, RP_EP_TCP, l2r_source, r2l_sink);

    plan(54
#ifdef USE_CHECK_WIRE
         + (2 * 8)
#endif /* USE_CHECK_WIRE */
#ifdef WITH_UINT8_T
         + 16
         + (2 * 2)
#endif /* WITH_UINT8_T */
         );

    /*
     * This test setup allows us to control the remote site (the thing trying
     * to access memory in a system), the local site (the system containing the
     * memory that gets accessed), as well as the data that is visible on the
     * communication channel, allowing to check not only the final observable
     * effect, but also the encoding of messages in the channel.
     *
     * To clarify the perspective of the specific test, we will use the
     * keywords "remote", "local" and "channel".
     *
     * Every conversation that is done is prefixed by a horizontal line
     * comment.
     */

    /* -------------------------------------------------------------------- */
    /* NOLINTBEGIN(concurrency-mt-unsafe) */
    {
        RPMaybeFrame mf;
        int rc;

        /* remote: Trigger a read request */
        rc = regp_req_read16(remote, 100, 1);
        okrc("remote: Sending read request signals success");

        /* channel: The read-request (including framing) should be on wire */

#ifdef USE_CHECK_WIRE
        t_check_wire(&r2l_buffer, (uint8_t[]){
                0x0cu,                      /* Length prefix */
                0x01u, 0x00u,               /* MOTV */
                0x00u, 0x00u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x64u, /* Address */
                0x00u, 0x00u, 0x00u, 0x01u  /* Block Size */
            }, 13, 0u);
#endif /* USE_CHECK_WIRE */

        /* local: Read frame from channel and parse it; this should be a read
         * request of course, and running the protocol processor on it, will
         * call out to the memory backend and emit a corresponding read respon-
         * se and put it onto the channel. */
        rc = regp_recv(local, &mf);
        okrc("local: Receiving read request signals success");
        okmf("local");
        ok(regp_is_read_request(mf.frame), "local: Frame is a read request");
        rc = regp_process(local, &mf);
        okrc("local: Processing read request signals success");
#ifdef USE_CHECK_WIRE
        t_check_wire(&l2r_buffer, (uint8_t[]){
                0x0eu,                      /* Length prefix */
                0x01u, 0x10u,               /* MOTV */
                0x00u, 0x00u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x64u, /* Address */
                0x00u, 0x00u, 0x00u, 0x01u, /* Block Size */
                0x64u, 0x00u                /* Payload */
            }, 15, 0u);
#endif /* USE_CHECK_WIRE */
        regp_free(local, mf.frame);

        /* channel: Now a read-response should be on the wire now. */

        /* remote: Read frame and parse it; this should yield a read response
         * of course and the payload should be equal to the requested memory.
         * Since we requested a single atom (16 bit here) from address 100 and
         * we initialised the memory so its data equals the address of the
         * memory, this should be 100 as well. */
        rc = regp_recv(remote, &mf);
        okrc("remote: Receiving read response signals success");
        okmf("remote");
        ok(regp_is_read_response(mf.frame), "remote: Frame is a read response");
        rc = regp_process(remote, &mf);
        okrc("remote: Processing read response signals success");
        with_good_mf(mf) {
            ok(mf.frame->header.address == 100u,
               "remote: Header reflects requested address (100)");
            ok(mf.frame->header.blocksize == 1u,
               "remote: Payload has expected size (1)");
            /* The memory transfered is uses the byte order of the machine it
             * was compiled for. So the native accessor is correct here. */
            const uint16_t datum = bf_ref_u16l(mf.frame->payload.data);
            ok(datum == 100u, "remote: Payload has expected value (100)");
        }

        regp_free(remote, mf.frame);

        /* Now lets modify an entry in the register table and see if that work
         * by reading it again. */

        uint16_t newvalue;
        bf_set_u16l(&newvalue, 0x100u);
        rc = regp_req_write16(remote, 100, 1, &newvalue);
        okrc("remote: Sending write request signals success");
#ifdef USE_CHECK_WIRE
        t_check_wire(&r2l_buffer, (uint8_t[]){
                0x0eu,                      /* Length prefix */
                0x01u, 0x20u,               /* MOTV */
                0x00u, 0x01u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x64u, /* Address */
                0x00u, 0x00u, 0x00u, 0x01u, /* Block Size */
                0x00u, 0x01u                /* Payload */
            }, 15, 0u);
#endif /* USE_CHECK_WIRE */
        /* We're sending two requests in a row. */
        rc = regp_req_read16(remote, 100, 1);
        okrc("remote: Sending read request signals success");
#ifdef USE_CHECK_WIRE
        t_check_wire(&r2l_buffer, (uint8_t[]){
                0x0cu,                      /* Length prefix */
                0x01u, 0x00u,               /* MOTV */
                0x00u, 0x02u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x64u, /* Address */
                0x00u, 0x00u, 0x00u, 0x01u, /* Block Size */
            }, 13, 15u);
#endif /* USE_CHECK_WIRE */

        rc = regp_recv(local, &mf);
        okrc("local: Receiving write request signals success");
        okmf("local");
        ok(regp_is_write_request(mf.frame), "local: Frame is a write request");
        rc = regp_process(local, &mf);
        okrc("local: Processing write request signals success");
#ifdef USE_CHECK_WIRE
        t_check_wire(&l2r_buffer, (uint8_t[]){
                0x0cu,                      /* Length prefix */
                0x01u, 0x30u,               /* MOTV */
                0x00u, 0x01u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x64u, /* Address */
                0x00u, 0x00u, 0x00u, 0x00u, /* Block Size */
            }, 13, 0u);
#endif /* USE_CHECK_WIRE */
        regp_free(local, mf.frame);
        /* The order in which we switch between operations on the remote and
         * local ends is inconsequential. All protocol memory accesses are
         * atomic from the point of view of the protocol. Doing it like this
         * just means that there will be two responses on the local-to-remote
         * wire for the remote side to read. */
        rc = regp_recv(local, &mf);
        okrc("local: Receiving read request signals success");
        okmf("local");
        ok(regp_is_read_request(mf.frame), "local: Frame is a read request");
        rc = regp_process(local, &mf);
        okrc("local: Processing read request signals success");
#ifdef USE_CHECK_WIRE
        t_check_wire(&l2r_buffer, (uint8_t[]){
                0x0eu,                      /* Length prefix */
                0x01u, 0x10u,               /* MOTV */
                0x00u, 0x02u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x64u, /* Address */
                0x00u, 0x00u, 0x00u, 0x01u, /* Block Size */
                0x00u, 0x01u                /* Payload */
            }, 15, 13u);
#endif /* USE_CHECK_WIRE */
        regp_free(local, mf.frame);

        /* Lets see if it worked. */
        rc = regp_recv(remote, &mf);
        okrc("remote: Receiving write response signals success");
        okmf("remote");
        ok(regp_is_write_response(mf.frame), "remote: Frame is a read response");
        rc = regp_process(remote, &mf);
        okrc("remote: Processing write response signals success");
        regp_free(local, mf.frame);

        rc = regp_recv(remote, &mf);
        okrc("remote: Receiving read response signals success");
        okmf("remote");
        ok(regp_is_read_response(mf.frame), "remote: Frame is a read response");
        rc = regp_process(remote, &mf);
        okrc("remote: Processing read response signals success");

        with_good_mf(mf) {
            ok(mf.frame->header.address == 100u,
               "remote: Header reflects requested address (100)");
            ok(mf.frame->header.blocksize == 1u,
               "remote: Payload has expected size (1)");
            const uint16_t datum = bf_ref_u16l(mf.frame->payload.data);
            ok(datum == 0x100u, "remote: Payload has expected value (0x100)");
        }

        regp_free(remote, mf.frame);

        /*
         * Perform a memory access that touches unmapped memory. This must
         * return a ReadResponse that has the ResponseType set to EUNMAPPED.
         * The first unmapped address must be in the payload of the response,
         * the WORD-SIZE-16 option must unset and the block-size parameter must
         * be set to 4 (octet semantics).
         */

        rc = regp_req_read16(remote, MEMORY_SIZE - 10, 20);
        okrc("remote: Sending read request signals success");
        rc = regp_recv(local, &mf);
        okrc("local: Receiving read request signals success");
        okmf("local");
        rc = regp_process(local, &mf);
        okrc("local: Processing read request signals success");
        regp_free(remote, mf.frame);

        rc = regp_recv(remote, &mf);
        okrc("remote: Receiving read response signals success");
        okmf("remote");
        with_good_mf (mf) {
            ok(mf.frame->header.type == RP_FRAME_READ_RESPONSE,
               "remote: Frame is a Read Response (%d)", mf.frame->header.type);
            ok(mf.frame->header.meta.response == RP_RESP_EUNMAPPED,
               "remote: Read Response indicates unmapped memory access (%d)",
               mf.frame->header.meta.response);
            ok(BIT_ISSET(mf.frame->header.options, RP_OPT_WORD_SIZE_16)==false,
               "remote: UnmappedMemory response has WORD-SIZE-16 unset");
            ok(mf.frame->header.blocksize == 4u,
               "remote: UnmappedMemory has blocksize == 4 (%zu)",
               mf.frame->header.blocksize);
        }
        regp_free(remote, mf.frame);

        ok(byte_buffer_rest(&l2r_buffer.buffer) == 0u,
           "channel: local-to-remote is empty");
        ok(byte_buffer_rest(&r2l_buffer.buffer) == 0u,
           "channel: remote-to-local is empty");

    }

#ifdef WITH_UINT8_T
    /* Switch to eight bit memory and perform a couple of interactions. */
    printf("# === Switching Local Memory to eight-bit Semantics ===\n");
    t_setup(false, RP_MEMTYPE_8, RP_EP_TCP);

    {
        RPMaybeFrame mf;
        int rc;

        rc = regp_req_read8(remote, 64, 1);
        okrc("remote: Sending read request signals success");

#ifdef USE_CHECK_WIRE
        t_check_wire(&r2l_buffer, (uint8_t[]){
                0x0cu,                      /* Length prefix */
                0x00u, 0x00u,               /* MOTV */
                0x00u, 0x00u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x40u, /* Address */
                0x00u, 0x00u, 0x00u, 0x01u  /* Block Size */
            }, 13, 0u);
#endif /* USE_CHECK_WIRE */

        rc = regp_recv(local, &mf);
        okrc("local: Receiving read request signals success");
        okmf("local");
        with_good_mf(mf) {
            ok(mf.frame->raw.size == 12, "local: Frame size: 12");
        }
        ok(regp_is_read_request(mf.frame), "local: Frame is a read request");
        rc = regp_process(local, &mf);
        okrc("local: Processing read request signals success");
#ifdef USE_CHECK_WIRE
        t_check_wire(&l2r_buffer, (uint8_t[]){
                0x0du,                      /* Length prefix */
                0x00u, 0x10u,               /* MOTV */
                0x00u, 0x00u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x40u, /* Address */
                0x00u, 0x00u, 0x00u, 0x01u, /* Block Size */
                0x40u                       /* Payload */
            }, 14, 0u);
#endif /* USE_CHECK_WIRE */
        regp_free(local, mf.frame);

        rc = regp_recv(remote, &mf);
        okrc("remote: Receiving read response signals success");
        okmf("remote");
        with_good_mf(mf) {
            ok(mf.frame->raw.size == 13, "local: Frame size: 12");
        }
        ok(regp_is_read_response(mf.frame), "remote: Frame is a read response");
        rc = regp_process(remote, &mf);
        okrc("remote: Processing read response signals success");
        with_good_mf(mf) {
            ok(mf.frame->header.address == 64u,
               "remote: Header reflects requested address (64)");
            ok(mf.frame->header.blocksize == 1u,
               "remote: Payload has expected size (1)");
            const uint8_t datum = *((uint8_t*)mf.frame->payload.data);
            ok(datum == 64u, "remote: Payload has expected value (64)");
        }

        regp_free(remote, mf.frame);

        ok(byte_buffer_rest(&l2r_buffer.buffer) == 0u,
           "channel: local-to-remote is empty");
        ok(byte_buffer_rest(&r2l_buffer.buffer) == 0u,
           "channel: remote-to-local is empty");
    }
#endif /* WITH_UINT8_T */

    /* Try a simple register transaction with the endpoint type set to
     * RP_EP_SERIAL. That will cause the system to use all the machinery for
     * serial channels, most notably SLIP for framing, and checksums. */
    printf("# === Switching Endpoint Kind to Serial ===\n");
    t_setup(false, RP_MEMTYPE_16, RP_EP_SERIAL);

    {
        RPMaybeFrame mf;
        int rc;
        rc = regp_req_read16(remote, 100, 1);
        okrc("remote: Sending read request signals success");
#ifdef USE_CHECK_WIRE
        t_check_wire(&r2l_buffer, (uint8_t[]){
                0x03u, 0x00u,               /* MOTV */
                0x00u, 0x00u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x64u, /* Address */
                0x00u, 0x00u, 0x00u, 0x01u, /* Block Size */
                0x0cu, 0xb4u,               /* Header Checksum */
                0xc0u                       /* End-of-Frame */
            }, 15, 0u);
#endif /* USE_CHECK_WIRE */
        rc = regp_recv(local, &mf);
        okrc("local: Receiving read request signals success");
        okmf("local");
        rc = regp_process(local, &mf);
        okrc("local: Processing read request signals success");
#ifdef USE_CHECK_WIRE
        t_check_wire(&l2r_buffer, (uint8_t[]){
                0x07u, 0x10u,               /* MOTV */
                0x00u, 0x00u,               /* SeqNo */
                0x00u, 0x00u, 0x00u, 0x64u, /* Address */
                0x00u, 0x00u, 0x00u, 0x01u, /* Block Size */
                0x8eu, 0x9du,               /* Header Checksum */
                0xdbu, 0xdcu, 0x2a,         /* Payload Checksum
                                             * c02a, SLIPped */
                0x64u, 0x00u,               /* Payload */
                0xc0u                       /* End-of-Frame */
            }, 20, 0u);
#endif /* USE_CHECK_WIRE */
        regp_free(local, mf.frame);
        rc = regp_recv(remote, &mf);
        okrc("remote: Receiving read response signals success");
        okmf("remote");
        with_good_mf(mf) {
            const uint16_t datum = bf_ref_u16l(mf.frame->payload.data);
            ok(datum == 100u, "remote: Payload has expected value (100)");
        }

        regp_free(remote, mf.frame);

        ok(byte_buffer_rest(&l2r_buffer.buffer) == 0u,
           "channel: local-to-remote is empty");
        ok(byte_buffer_rest(&r2l_buffer.buffer) == 0u,
           "channel: remote-to-local is empty");
    }
    /* NOLINTEND(concurrency-mt-unsafe) */

    return EXIT_SUCCESS;
}
