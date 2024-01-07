/*
 * Copyright (c) 2023-2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <ufw/allocator.h>
#include <ufw/binary-format.h>
#include <ufw/compat/errno.h>
#include <ufw/compiler.h>
#include <ufw/crc/crc16-arc.h>
#include <ufw/endpoints.h>
#include <ufw/endpoints/continuable-sink.h>
#include <ufw/length-prefix.h>
#include <ufw/register-protocol.h>
#include <ufw/rfc1055.h>

#define RP_HEADER_SIZE_16      8u
#define RP_HEADER_MIN_SIZE_16  6u

#define RP_HEADER_SIZE_8      (RP_HEADER_SIZE_16     * 2u)
#define RP_HEADER_MIN_SIZE_8  (RP_HEADER_MIN_SIZE_16 * 2u)

#define RP_HEADER_SIZE        (RP_HEADER_SIZE_16     * sizeof(uint16_t))
#define RP_HEADER_MIN_SIZE    (RP_HEADER_MIN_SIZE_16 * sizeof(uint16_t))

#define MSEM_AUTO  0u
#define MSEM_8BIT  1u
#define MSEM_16BIT 2u

/*
 * Internal Utilities
 */

static inline bool
raw_with_hdcrc(const uint16_t motv)
{
    return BIT_ISSET(motv, RP_OPT_WITH_HEADER_CRC << 8u);
}

static inline bool
raw_with_plcrc(const uint16_t motv)
{
    return BIT_ISSET(motv, RP_OPT_WITH_PAYLOAD_CRC << 8u);
}

static inline uint16_t
make_motv(const RegP *p, const unsigned int msem,
          const uint_least8_t meta,
          const RPFrameType type, const size_t n)
{
    uint16_t rv = RP_IMPLEMENTATION_VERSION & 0x0fu;
    rv |= (type & 0x0fu) << 4u;
    rv |= ( (((msem == MSEM_AUTO && p->memory.type == RP_MEMTYPE_16)
              || msem == MSEM_16BIT)
             ? RP_OPT_WORD_SIZE_16
             : 0u)
          | (p->ep.type == RP_EP_SERIAL
             ? RP_OPT_WITH_HEADER_CRC
             : 0u)
          | ((p->ep.type == RP_EP_SERIAL
              && n > 0
              && type != RP_FRAME_READ_REQUEST)
             ? RP_OPT_WITH_PAYLOAD_CRC
             : 0u))
        << 8u;
    rv |= meta << 12u;
    return rv;
}

static inline void
populate_header(uint16_t *buf, RegP *p,
                const unsigned int msem,
                const RPFrameType type,
                const uint_least8_t meta,
                const uint16_t seqno,
                const uint32_t address,
                const size_t n,
                const uint16_t plcrc)
{
    bf_set_u16b(buf, make_motv(p, msem, meta, type, n));
    bf_set_u16b(buf + 1, seqno);
    bf_set_u32b(buf + 2, address);
    bf_set_u32b(buf + 4, n);
    buf[6] = 0u;
    bf_set_u16b(buf + 7, plcrc);
}

static size_t
encode_header(uint16_t *buf, RegP *p,
              const unsigned int msem,
              const RPFrameType type,
              const uint_least8_t meta,
              const uint16_t seqno,
              const uint32_t address,
              const size_t n,
              const uint16_t plcrc)
{
    populate_header(buf, p, msem, type, meta, seqno, address, n, plcrc);
    const uint16_t motv = bf_ref_u16b(buf);
    const bool with_hdcrc = raw_with_hdcrc(motv);
    const bool with_plcrc = raw_with_plcrc(motv);
    size_t size = RP_HEADER_MIN_SIZE_16;
    uint16_t crc = 0u;

    if (with_hdcrc) {
        crc = ufw_buffer_crc16_arc_u16(buf, RP_HEADER_MIN_SIZE_16);
        if (with_plcrc) {
            crc = ufw_crc16_arc_u16(crc, buf + RP_HEADER_SIZE_16 - 1u, 1u);
        }
        bf_set_u16b(buf + RP_HEADER_MIN_SIZE_16, crc);
        size++;
    }

    if (with_plcrc) {
        size++;
    }

    return size;
}

static int
parse_header(RPFrame *frame, void *buf, size_t n)
{
    if (n < RP_HEADER_MIN_SIZE) {
        return -EBADMSG;
    }

    uint16_t *raw = buf;
    uint16_t motv = bf_ref_u16b(raw);

    frame->header.version = BIT_GET(motv, 4u, 0u);
    if (frame->header.version != RP_IMPLEMENTATION_VERSION) {
        return -EBADMSG;
    }

    frame->header.type = (RPFrameType)BIT_GET(motv, 4u, 4u);
    frame->header.options = BIT_GET(motv, 4u, 8u);
    if ((frame->header.options & 0x8u) != 0u) {
        return -EBADMSG;
    }

    frame->header.meta.raw = BIT_GET(motv, 4u, 12u);
    switch (frame->header.type) {
    case RP_FRAME_READ_REQUEST:
        /* FALLTHROUGH */
    case RP_FRAME_WRITE_REQUEST:
        if (frame->header.meta.raw != 0u) {
            return -EBADMSG;
        }
        break;
    case RP_FRAME_READ_RESPONSE:
        /* FALLTHROUGH */
    case RP_FRAME_WRITE_RESPONSE:
        if (frame->header.meta.raw > RP_RESP_EINVALID) {
            return -EBADMSG;
        }
        break;
    case RP_FRAME_META:
        if (frame->header.meta.raw < 1 || frame->header.meta.raw > 2) {
            return -EBADMSG;
        }
        break;
    default:
        return -EBADMSG;
    }

    frame->header.sequence  = bf_ref_u16b(raw + 1);
    frame->header.address   = bf_ref_u32b(raw + 2);
    frame->header.blocksize = bf_ref_u32b(raw + 4);
    frame->header.hdcrc = 0;
    frame->header.plcrc = 0;

    const bool with_hdcrc = raw_with_hdcrc(motv);
    const bool with_plcrc = raw_with_plcrc(motv);
    unsigned int offset = 6u;
    uint16_t crc = 0u;

    if (with_hdcrc && with_plcrc && n < RP_HEADER_SIZE) {
        return -EBADMSG;
    }

    if ((with_hdcrc || with_plcrc) && n < (RP_HEADER_SIZE - sizeof(uint16_t)))
    {
        return -EBADMSG;
    }

    if (with_hdcrc) {
        frame->header.hdcrc = bf_ref_u16b(raw + offset);
        crc = ufw_buffer_crc16_arc_u16(raw, offset);
        if (with_plcrc) {
            crc = ufw_crc16_arc_u16(crc, raw + RP_HEADER_SIZE_16 - 1u, 1u);
        }
        offset++;
    }
    if (with_plcrc) {
        frame->header.plcrc = bf_ref_u16b(raw + offset);
        offset++;
    }

    return (crc == frame->header.hdcrc) ? (int)offset : -EILSEQ;
}

static int
check_payload(const RPFrame *f)
{
    if (regp_has_hdcrc(f) == false || f->payload.size == 0u) {
        return 0;
    }

    uint16_t crc = 0u;

    if (BIT_ISSET(f->header.options, RP_OPT_WORD_SIZE_16)) {
        crc = ufw_buffer_crc16_arc_u16(f->payload.data, f->header.blocksize);
    } else {
#ifdef WITH_UINT8_T
        crc = ufw_buffer_crc16_arc(f->payload.data, f->header.blocksize);
#else
        return -EINVAL;
#endif /* WITH_UINT8_T */
    }

    return (crc == f->header.plcrc) ? 0 : -EPROTO;
}

static int
payload_plausible(RPFrame *f)
{
    size_t actualsize = f->payload.size;
    if (BIT_ISSET(f->header.options, RP_OPT_WORD_SIZE_16)) {
        actualsize /= 2;
    }
    switch (f->header.type) {
    case RP_FRAME_READ_REQUEST:
        /* FALLTHROUGH */
    case RP_FRAME_WRITE_RESPONSE:
        /* FALLTHROUGH */
    case RP_FRAME_META:
        return (actualsize == 0) ? 0 : -EFAULT;
    case RP_FRAME_READ_RESPONSE:
        /* FALLTHROUGH */
    case RP_FRAME_WRITE_REQUEST:
        return (f->header.blocksize == actualsize) ? 0 : -EFAULT;
    default:
        return -EINVAL;
    }
}

static int
parse_frame(ByteBuffer *framebuf)
{
    int rc;

    RPFrame *frame = (RPFrame*)framebuf->data;
    frame->raw.memory = framebuf->data + sizeof(RPFrame);
    frame->raw.size = framebuf->used - sizeof(RPFrame);

    rc = parse_header(frame, frame->raw.memory, frame->raw.size);

    if (rc < 0) {
        return rc;
    }

    frame->payload.data = (uint16_t*)frame->raw.memory + rc;
    frame->payload.size = frame->raw.size - ((unsigned char*)frame->payload.data
                                          -  (unsigned char*)frame->raw.memory);


    rc = payload_plausible(frame);
    if (rc < 0) {
        return rc;
    }

    rc = check_payload(frame);
    if (rc < 0) {
        return rc;
    }

    return 0;
}

static int
send_memory(RegP *p, void *hdr, size_t hs, void *pl, size_t ps)
{
    ByteBuffer chunks[] = {
        BYTE_BUFFER(hdr, hs),
        BYTE_BUFFER(pl, ps)
    };
    ByteChunks data = BYTE_CHUNKS(chunks);
    if (pl == NULL) {
        data.chunks = 1u;
    }

    /* Apply the correct framing here. */
    switch (p->ep.type) {
    case RP_EP_TCP: {
        const ssize_t rc = lenp_chunks_to_sink(&p->ep.sink, &data);
        return (rc < 0) ? (int)rc : 0;
    }
    case RP_EP_SERIAL:
        /* FALLTHROUGH */
    default: {
        RFC1055Context slip = RFC1055_CONTEXT_INIT_DEFAULT;
        Source src;
        source_from_chunks(&src, &data);
        return rfc1055_encode(&slip, &src, &p->ep.sink);
    }
    }
}

static RPFrameType
req2resp(const RPFrameType type)
{
    switch (type) {
    case RP_FRAME_READ_REQUEST:  return RP_FRAME_READ_RESPONSE;
    case RP_FRAME_WRITE_REQUEST: return RP_FRAME_WRITE_RESPONSE;
    default:                     return RP_FRAME_META;
    }
}

static int
send_resp_0(RegP *p, const RPFrame *frame, const RPResponse code,
            const unsigned int msem)
{
    uint16_t header[RP_HEADER_SIZE_16];
    const size_t size = sizeof(uint16_t) *
        encode_header(header, p, msem, req2resp(frame->header.type), code,
                      frame->header.sequence, frame->header.address, 0u, 0u);
    return send_memory(p, header, size, NULL, 0u);
}

static inline size_t
msem_size(RegP *p, const unsigned int msem, const size_t n)
{
    switch (msem) {
    case MSEM_16BIT: return n;
    case MSEM_8BIT:  return n * 2u;
    default:         return n * (p->memory.type == RP_MEMTYPE_16 ? 1u : 2u);
    }
}

static int
send_resp_32(RegP *p, const RPFrame *frame, RPResponse code, const uint32_t pl,
             const unsigned int msem)
{
    uint16_t header[RP_HEADER_SIZE_16 + 2];
    uint16_t *plbuf = header + RP_HEADER_SIZE_16;
    bf_set_u32b(plbuf, pl);
    const uint16_t plcrc = ufw_buffer_crc16_arc_u16(plbuf, 2u);
    const size_t size = sizeof(uint16_t) *
        encode_header(header, p, msem, req2resp(frame->header.type), code,
                      frame->header.sequence, frame->header.address,
                      msem_size(p, msem, 2), plcrc);
    uint32_t realpl;
    bf_set_u32b(&realpl, pl);
    return send_memory(p, header, size, &realpl, sizeof(realpl));
}

/* Handle early errors. These functions are called when headers have not been
 * parsed yet. They parse a header from a buffer and emit a response. This
 * really should only happen in regp_recv(). Normal code should use
 * regp_resp_ebusy() and regp_resp_erxoverflow() instead. */

static int
send_early_response(RegP *p, ByteBuffer *hdrbuf, RPResponse code)
{
    RPFrame frame;
    const int rc = parse_header(&frame, hdrbuf->data, hdrbuf->used);

    if (rc >= 0) {
        return send_resp_0(p, &frame, code, MSEM_8BIT);
    }

    if (rc == -EBADMSG) {
        return regp_resp_meta(p, RP_META_EHEADERENC);
    }

    if (rc == -EILSEQ) {
        return regp_resp_meta(p, RP_META_EHEADERCRC);
    }

    return rc;
}

static int
early_ebusy(RegP *p, ByteBuffer *hdrbuf)
{
    return send_early_response(p, hdrbuf, RP_RESP_EBUSY);
}

static int
early_erxoverflow(RegP *p, ByteBuffer *hdrbuf)
{
    return send_early_response(p, hdrbuf, RP_RESP_ERXOVERFLOW);
}

static inline bool
memtype_valid(const RegP *p, const RPFrame *f)
{
    const bool opt16 = BIT_ISSET(f->header.options, RP_OPT_WORD_SIZE_16);
    return ((p->memory.type == RP_MEMTYPE_16 && opt16) ||
            (p->memory.type == RP_MEMTYPE_8 && opt16 == false));
}

static void
setup_buffer(ByteBuffer *b)
{
    assert(sizeof(RPFrame) < b->size);
    b->used = sizeof(RPFrame);
}

static inline size_t
trxbufsize(const RegP *p)
{
    /* Including binary-format.h ensures CHAR_BIT is either 8 or 16. */
    const size_t bs = p->alloc->blocksize * (CHAR_BIT / 8);
    const size_t fs = sizeof(RPFrame)     * (CHAR_BIT / 8);
    return bs - fs;
}

static inline uint32_t
address_min(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

static inline uint32_t
address_max(uint32_t a, uint32_t b)
{
    return (a > b) ? a : b;
}

static inline uint32_t
range_start(const RPRange *b)
{
    return (b->address);
}

static inline uint32_t
range_end(const RPRange *b)
{
    return (b->address + b->size - 1);
}

/*
 * Default instance interfaces
 *
 * These are a malloc based block allocator and a memory interface that returns
 * EUNMAPPED for every access.
 */

BlockAllocator rp_default_allocator =
    MAKE_STDHEAD_BLOCKALLOC(RP_DEFAULT_BUFFER_SIZE);

RPBlockAccess
regp_void_read16(uint32_t address,
                 UNUSED size_t bsize,
                 UNUSED uint16_t *buf)
{
    RPBlockAccess rv = { .status = RP_RESP_EUNMAPPED, .address = address };
    return rv;
}

RPBlockAccess
regp_void_write16(uint32_t address,
                  UNUSED size_t bsize,
                  UNUSED const uint16_t *buf)
{
    RPBlockAccess rv = { .status = RP_RESP_EUNMAPPED, .address = address };
    return rv;
}

/*
 * Initialisation / Post-instantiation specialisation
 *
 * These allow the modification of the block allocator, connect the system to a
 * data channel that can actually do something, and slot in a non-void memory
 * implementation.
 */

void
regp_init(RegP *p)
{
    p->memory.type = RP_MEMTYPE_16;
    p->memory.access.m16.read = regp_void_read16;
    p->memory.access.m16.write = regp_void_write16;
    p->session.sequence = 0u;
    p->ep.type = RP_EP_TCP;
    p->ep.source = source_empty;
    p->ep.sink = sink_null;
    p->alloc = &rp_default_allocator;
}

#ifdef WITH_UINT8_T
void
regp_use_memory8(RegP *p, RPBlockRead8 read, RPBlockWrite8 write)
{
    p->memory.type = RP_MEMTYPE_8;
    p->memory.access.m8.read = read;
    p->memory.access.m8.write = write;
}
#endif /* WITH_UINT8_T */

void
regp_use_memory16(RegP *p, RPBlockRead16 read, RPBlockWrite16 write)
{
    p->memory.type = RP_MEMTYPE_16;
    p->memory.access.m16.read = read;
    p->memory.access.m16.write = write;
}

void
regp_use_channel(RegP *p, RPEndpointType type, Source source, Sink sink)
{
    p->ep.type = type;
    p->ep.source = source;
    p->ep.sink = sink;
}

void
regp_use_allocator(RegP *p, BlockAllocator *alloc)
{
    p->alloc = alloc;
}

/*
 * Request API
 *
 * These can be used to perform the protocol's primitives, namely block
 * transfers in the form of read and write accesses.
 */

int
regp_req_read8(RegP *p, uint32_t address, size_t n)
{
    uint16_t header[RP_HEADER_SIZE_16];
    const size_t size = sizeof(uint16_t) *
        encode_header(header, p, MSEM_8BIT,
                      RP_FRAME_READ_REQUEST, 0u, p->session.sequence,
                      address, n, 0u);
    p->session.sequence++;
    return send_memory(p, header, size, NULL, 0u);
}

int
regp_req_read16(RegP *p, uint32_t address, size_t n)
{
    uint16_t header[RP_HEADER_SIZE_16];
    const size_t size = sizeof(uint16_t) *
        encode_header(header, p, MSEM_16BIT,
                      RP_FRAME_READ_REQUEST, 0u, p->session.sequence,
                      address, n, 0u);
    p->session.sequence++;
    return send_memory(p, header, size, NULL, 0u);
}

#ifdef WITH_UINT8_T
int
regp_req_write8(RegP *p, const uint32_t address, const size_t n,
                const uint8_t *buf)
{
    uint16_t header[RP_HEADER_SIZE_16];
    const uint16_t plcrc = ufw_buffer_crc16_arc(buf, n);
    const size_t size = sizeof(uint16_t) *
        encode_header(header, p, MSEM_8BIT,
                      RP_FRAME_WRITE_REQUEST, 0u, p->session.sequence,
                      address, n, plcrc);
    p->session.sequence++;
    return send_memory(p, header, size, (void*)buf, n * sizeof(*buf));
}
#endif /* WITH_UINT8_T */

int
regp_req_write16(RegP *p, const uint32_t address, const size_t n,
                 const uint16_t *buf)
{
    uint16_t header[RP_HEADER_SIZE_16];
    const uint16_t plcrc = ufw_buffer_crc16_arc_u16(buf, n);
    const size_t size = sizeof(uint16_t) *
        encode_header(header, p, MSEM_16BIT,
                      RP_FRAME_WRITE_REQUEST, 0u, p->session.sequence,
                      address, n, plcrc);
    p->session.sequence++;
    return send_memory(p, header, size, (void*)buf, n * sizeof(*buf));
}

/*
 * Response API
 *
 * These functions can be used to emit all kinds of protocol responses. These
 * should only be used to implement special memory behaviour on top of the
 * standard block memory behaviour. All normal behaviour is implemented by
 * regp_process(), which should be used in the vast majority (in some instances
 * all) cases.
 */

int
regp_resp_ack(RegP *p, const RPFrame *f, const void *pl, const size_t n)
{
    uint16_t header[RP_HEADER_SIZE_16];
    uint16_t plcrc = 0u;
    size_t plsize = 0u;

    if (p->memory.type == RP_MEMTYPE_16) {
        plcrc = pl != NULL ? ufw_buffer_crc16_arc_u16(pl, n) : 0u;
        plsize = n * sizeof(uint16_t);
    } else {
#ifdef WITH_UINT8_T
        plcrc = pl != NULL ? ufw_buffer_crc16_arc(pl, n) : 0u;
        plsize = n;
#else
        return -EINVAL;
#endif /* WITH_UINT8_T */
    }
    const size_t size = sizeof(uint16_t) *
        encode_header(header, p, MSEM_AUTO,
                      req2resp(f->header.type), 0u, f->header.sequence,
                      f->header.address, n, plcrc);
    return send_memory(p, header, size, (void*)pl, plsize);
}

int
regp_resp_ewordsize(RegP *p, const RPFrame *f)
{
    return send_resp_0(p, f, RP_RESP_EWORDSIZE, MSEM_8BIT);
}

int
regp_resp_epayloadcrc(RegP *p, const RPFrame *f)
{
    return send_resp_0(p, f, RP_RESP_EPAYLOADCRC, MSEM_8BIT);
}

int
regp_resp_epayloadsize(RegP *p, const RPFrame *f)
{
    return send_resp_0(p, f, RP_RESP_EPAYLOADSIZE, MSEM_8BIT);
}

int
regp_resp_erxoverflow(RegP *p, const RPFrame *f, const uint32_t size)
{
    return send_resp_32(p, f, RP_RESP_ERXOVERFLOW, size, MSEM_8BIT);
}

int
regp_resp_etxoverflow(RegP *p, const RPFrame *f, const uint32_t size)
{
    return send_resp_32(p, f, RP_RESP_ETXOVERFLOW, size, MSEM_8BIT);
}

int
regp_resp_ebusy(RegP *p, const RPFrame *f)
{
    return send_resp_0(p, f, RP_RESP_EBUSY, MSEM_8BIT);
}

int
regp_resp_eunmapped(RegP *p, const RPFrame *f, const uint32_t address)
{
    return send_resp_32(p, f, RP_RESP_EUNMAPPED, address, MSEM_8BIT);
}

int
regp_resp_eaccess(RegP *p, const RPFrame *f, const uint32_t address)
{
    return send_resp_32(p, f, RP_RESP_EACCESS, address, MSEM_8BIT);
}

int
regp_resp_erange(RegP *p, const RPFrame *f, const uint32_t address)
{
    return send_resp_32(p, f, RP_RESP_ERANGE, address, MSEM_8BIT);
}

int
regp_resp_einvalid(RegP *p, const RPFrame *f, const uint32_t address)
{
    return send_resp_32(p, f, RP_RESP_EINVALID, address, MSEM_8BIT);
}

int
regp_resp_eio(RegP *p, const RPFrame *f)
{
    return send_resp_0(p, f, RP_RESP_EIO, MSEM_8BIT);
}

int
regp_resp_meta(RegP *p,const uint_least8_t meta)
{
    uint16_t header[RP_HEADER_SIZE_16];
    const size_t size = sizeof(uint16_t) *
        encode_header(header, p, MSEM_8BIT,
                      RP_FRAME_META, meta, 0u, 0u, 0u, 0u);
    return send_memory(p, header, size, NULL, 0u);
}

/*
 * Processing API
 *
 * Communication endpoints, that respond to requests specified in the protocol
 * can use regp_recv() to receive frames from the source hooked into the proto-
 * col instance. regp_process() implements the standard response behaviour.
 *
 * regp_recv() allocates memory for the frame it returns. You have to use
 * regp_free() when the returned data is not needed anymore to avoid memory
 * leaks.
 */

int
regp_recv(RegP *p, RPMaybeFrame *mf)
{
    /*
     * This function has two jobs: Receive a frame and parse it, to be able to
     * hand it to the user.
     *
     * The system allocates memory for that RPFrame instance. If it cannot do
     * that, we cannot process incoming data at this point in time. In that
     * case, the frame should still be read, but only to be able to send an
     * EBUSY response.
     *
     * Similarly, if a buffer could be allocated, but a frame is to large to
     * fit into this memory block, the frame should still be read, but only to
     * be able to a ERXOVERFLOW response.
     *
     * One way to do this would be to employ some sort of polling mechanism, to
     * check if there is data available in the Source registered with the RegP
     * instance. But currently, the Source interface has no such property, as
     * there is no portable way in ufw to do this sort of polling. So that is
     * not the way to go — not now, and arguably not ever. Really a thread
     * serving a connection of this protocol should basically be able to just
     * call:
     *
     *     for (;;) {
     *         rc = regp_recv(p, &mf);
     *         if (rc < 0) {
     *             error_handling_here();
     *         }
     *         if (maybe_custom_processing_here() == true) {
     *             regp_free(mf.frame);
     *             continue;
     *         }
     *         rc = regp_process(p, &mf);
     *         if (rc < 0) {
     *             more_error_handling_here();
     *         }
     *         regp_free(mf.frame);
     *     }
     *
     * ...and have all the blocking be done in the sources and sinks involved
     * in the system. This would be very straight forward code, with all end-
     * points and system dependencies being nicely encapsulated.
     *
     * To achieve this, we will implement a special kind of sink, that will
     * keep accepting data, even though its buffers are filled already. It
     * allows us to detect the error conditions and react accordingly.
     *
     * If all goes well (allocation succeeds, and the frame fits in the buffer)
     * what needs to be done is this:
     *
     * The complete raw frame will be at offset sizeof(RPFrame) inside of the
     * frame buffer in the sink driver. The parser needs to read data from this
     * memory section and populate the members of RPFrame, including the
     * raw.memory and payload.data pointers. This step also performs any
     * checksum checks that are required.
     *
     * If the header of the message fails to parse, the system sends a
     * EHEADERENC META frame to the remote side. The error.errno value in
     * maybe_frame will be set to EBADMSG.
     *
     * If the header checksum could not be verified a EHEADERCRC META frame is
     * sent to the remote site. error.errno is set to EILSEQ.
     *
     * These are the steps that need to be performed here, because the data
     * tested here is so fundamental, that no valid frame can be formed if any
     * of these fail. If they pass, the frame's header is okay. The system will
     * perform two additional tests:
     *
     * - Does the payload size from the header match the payload in the frame?
     *   If not, set error.errno to EFAULT.
     *
     * - Does the payload CRC check out? If not, set error.errno to EPROTO.
     *
     * With all of these tests, if error.errno is zero:
     *
     * - A frame was read completely into a freshly allocated buffer.
     * - Its header was parsed completely, and
     * - its checksum was verified.
     * - Its payload (if any) matches the size advertised in the header, and
     * - its checksum was verified.
     *
     * As far as the protocol is concerned, this is a completely valid message,
     * that can be used for further processing. The default processing will be
     * done via regp_process(). Users can put in custom logic in between
     * regp_recv() and regp_process() if they so desire.
     *
     * Summary of mf->error.id semantics:
     *
     * - EBUSY:   Allocation of a frame buffer failed.
     * - ENOMEM:  Frame buffer was too small to store incoming frame.
     * - EBADMSG: Parsing the header failed.
     * - EILSEQ:  Verifying the header checksum failed.
     * - EFAULT:  Payload size is implausible, considering block size in header.
     * - EPROTO:  Verifying the payload checksum failed.
     */
    mf->frame = NULL;
    mf->error.id = 0;
    mf->error.framesize = 0u;
    uint16_t fallback[RP_HEADER_SIZE_16];
    ByteBuffer fb = BYTE_BUFFER_EMPTY((void*)fallback, sizeof(fallback));
    Sink recv;
    ContinuableSink cs = CONTINUABLE_SINK(p->alloc, &fb, setup_buffer);
    continuable_sink_init(&recv, &cs);

    switch (p->ep.type) {
    case RP_EP_TCP: {
        const ssize_t rc = lenp_decode_source_to_sink(&p->ep.source, &recv);
        if (rc < 0) {
            return rc;
       }
    } break;
    case RP_EP_SERIAL:
        /* FALLTHROUGH */
    default: {
        RFC1055Context slip = RFC1055_CONTEXT_INIT_DEFAULT;
        const int rc = rfc1055_decode(&slip, &p->ep.source, &recv);
        if (rc < 0) {
            return rc;
        }
    } break;
    }

    if (cs.error.id != 0) {
        mf->error.id = cs.error.id;
        mf->error.framesize = cs.error.datacount;
    }

    mf->frame = (RPFrame*)cs.buffer.data;

    switch (cs.error.id) {
    case 0:
        /* No error indicated. Good! */
        break;
    case EBUSY:
        /* Send EBUSY reply, based on fallback buffer */
        return early_ebusy(p, &fb);
    case ENOMEM:
        /* Send ERXOVERFLOW reply, based on fallback buffer */
        byte_buffer_rewind(&fb);
        byte_buffer_add(&fb, mf->frame->raw.memory, RP_HEADER_SIZE);
        return early_erxoverflow(p, &fb);
    default:
        /* Unexpected error. Really shouldn't happen. */
        return -EINVAL;
    }

    int rc = parse_frame(&cs.buffer);

    if (rc < 0) {
        mf->error.id = -rc;
    }

    if (rc == -EBADMSG) {
        return regp_resp_meta(p, RP_META_EHEADERENC);
    }

    if (rc == -EILSEQ) {
        return regp_resp_meta(p, RP_META_EHEADERCRC);
    }

    /* Frame read, parsed and verified. Nice. */
    return 0;
}

int
regp_process(RegP *p, const RPMaybeFrame *mf)
{
    /*
     * This function is very lenient in its frame parameter: If you feed a
     * result that indicated a fatal problem in regp_recv() into this function,
     * it will just ignore the parameter and return immediately.
     *
     * Beyond that, this deals with mostly well structured frames. It may still
     * take care of a couple of error cases:
     *
     * - Invalid payload CRC: Send EPAYLOADCRC response.
     * - Implausible payload size: Send EPAYLOADSIZE response.
     * - Incompatible memory word size: send EWORDSIZE response.
     *
     * If none if those apply, we're dealing with a well structured and (from
     * the protocol's point of view) valid frame. If this is not a request
     * frame, the function ignores the frame and returns.
     *
     * If the frame is a request frame, the primitive action is forwarded to
     * the memory connected to the RegP instance and the result of that action
     * is returned to the remote side with the appropriate response type (read-
     * or write-response).
     */

    if (mf->frame == NULL) {
        /* This indicates a very early error. Can't do any further processing
         * so we ignore it. We test this here so the function can count on this
         * being available. */
        return 0;
    }

    switch (mf->error.id) {
    case 0:
        /* In the successful case (0) there is no further error handling to do
         * here and this function can continue. */
        break;
    case EPROTO:
        if (regp_is_request(mf->frame)) {
            return send_resp_0(p, mf->frame, RP_RESP_EPAYLOADCRC, MSEM_8BIT);
        }
        return 0;

    case EFAULT:
        if (regp_is_request(mf->frame)) {
            return send_resp_0(p, mf->frame, RP_RESP_EPAYLOADSIZE, MSEM_8BIT);
        }
        return 0;
    default:
        /* In the other error-cases, regp_recv() will have done the error
         * handling already. This function is lenient about this and just
         * returns here, since a frame that caused an early error cannot be a
         * valid request, that can be forwarded to the memory implementa-
         * tion. */
        return 0;
    }

    if (regp_is_request(mf->frame) == false) {
        /* Even if a frames is valid, if it is not a request type frame, there
         * is no further processing to be done. */
        return 0;
    }

    if (memtype_valid(p, mf->frame) == false) {
        /* Finally if the memory type of the implementation is not compatible
         * with the type indicated by the request, fail the request here. */
        return send_resp_0(p, mf->frame, RP_RESP_EWORDSIZE, MSEM_8BIT);
    }

    /* When we're here, the provided frame is completely valid and contains a
     * request for our memory implementation to serve. Let's forward it. */

    RPBlockAccess ba;
    const uint32_t addr = mf->frame->header.address;
    const size_t blocksize = mf->frame->header.blocksize;
    void *buf = mf->frame->payload.data;

    if (regp_is_read_request(mf->frame)) {
        /* In read-requests, the frame doesn't carry any payload. We'll use the
         * block's memory after the header in order to store the return data
         * from the memory implementation. This removes the requirement of
         * allocating again, and eliminates some block memory waste. */
        if (p->memory.type == RP_MEMTYPE_16) {
            const size_t maxsize = (p->alloc->blocksize - sizeof(RPFrame)) / 2;
            if (maxsize < blocksize) {
                ba.status = RP_RESP_ETXOVERFLOW;
            } else {
                ba = p->memory.access.m16.read(addr, blocksize, buf);
            }
        } else {
            const size_t maxsize = p->alloc->blocksize - sizeof(RPFrame);
            if (maxsize < blocksize) {
                ba.status = RP_RESP_ETXOVERFLOW;
            } else {
#ifdef WITH_UINT8_T
                ba = p->memory.access.m8.read(addr, blocksize, buf);
#else
                ba.status = RP_RESP_EWORDSIZE;
#endif /* WITH_UINT8_T */
            }
        }
    } else { /* Write request */
        if (p->memory.type == RP_MEMTYPE_16) {
            ba = p->memory.access.m16.write(addr, blocksize, buf);
        } else {
#ifdef WITH_UINT8_T
            ba = p->memory.access.m8.write(addr, blocksize, buf);
#else
            ba.status = RP_RESP_EWORDSIZE;
#endif /* WITH_UINT8_T */
        }
        buf = NULL;
    }

    /* What's left now is to produce the correct response message given the
     * request and the block access status returned by the accessors. */

    switch (ba.status) {
    case RP_RESP_ACK:
        return regp_resp_ack(p, mf->frame, buf, buf == NULL ? 0u : blocksize);
    case RP_RESP_EWORDSIZE:
        return regp_resp_ewordsize(p, mf->frame);
    case RP_RESP_EPAYLOADCRC:
        return regp_resp_epayloadcrc(p, mf->frame);
    case RP_RESP_EPAYLOADSIZE:
        return regp_resp_epayloadsize(p, mf->frame);
    case RP_RESP_ERXOVERFLOW:
        return regp_resp_erxoverflow(p, mf->frame, trxbufsize(p));
    case RP_RESP_ETXOVERFLOW:
        return regp_resp_etxoverflow(p, mf->frame, trxbufsize(p));
    case RP_RESP_EBUSY:
        return regp_resp_ebusy(p, mf->frame);
    case RP_RESP_EUNMAPPED:
        return regp_resp_eunmapped(p, mf->frame, ba.address);
    case RP_RESP_EACCESS:
        return regp_resp_eaccess(p, mf->frame, ba.address);
    case RP_RESP_ERANGE:
        return regp_resp_erange(p, mf->frame, ba.address);
    case RP_RESP_EINVALID:
        return regp_resp_einvalid(p, mf->frame, ba.address);
    case RP_RESP_EIO:
        return regp_resp_eio(p, mf->frame);
    default:
        /* This shouldn't happen. */
        return -EINVAL;
    }
}

/*
 * Matching API
 *
 * This part of the API allows case analysis on frame instances. This can be
 * used to implement special system behaviour on top of the normal one imple-
 * mented by regp_process().
 */

bool
regp_is_valid(const RPFrame *f)
{
    return (f != NULL && f->header.type != RP_FRAME_INVALID);
}

bool
regp_is_request(const RPFrame *f)
{
    return (f != NULL &&
            (f->header.type == RP_FRAME_READ_REQUEST ||
             f->header.type == RP_FRAME_WRITE_REQUEST));
}

bool
regp_is_response(const RPFrame *f)
{
    return (f != NULL &&
            (f->header.type == RP_FRAME_READ_RESPONSE ||
             f->header.type == RP_FRAME_WRITE_RESPONSE));
}

bool
regp_is_read_request(const RPFrame *f)
{
    return (f != NULL && f->header.type == RP_FRAME_READ_REQUEST);
}

bool
regp_is_write_request(const RPFrame *f)
{
    return (f != NULL && f->header.type == RP_FRAME_WRITE_REQUEST);
}

bool
regp_is_read_response(const RPFrame *f)
{
    return (f != NULL && f->header.type == RP_FRAME_READ_RESPONSE);
}

bool
regp_is_write_response(const RPFrame *f)
{
    return (f != NULL && f->header.type == RP_FRAME_WRITE_RESPONSE);
}

bool
regp_is_meta_message(const RPFrame *f)
{
    return (f != NULL && f->header.type == RP_FRAME_META);
}

bool
regp_is_16bitsem(const RPFrame *f)
{
    return BIT_ISSET(f->header.options, RP_OPT_WORD_SIZE_16);
}

bool
regp_has_hdcrc(const RPFrame *f)
{
    return BIT_ISSET(f->header.options, RP_OPT_WITH_HEADER_CRC);
}

bool
regp_has_plcrc(const RPFrame *f)
{
    return BIT_ISSET(f->header.options, RP_OPT_WITH_PAYLOAD_CRC);
}

/*
 * Miscellaneous API
 *
 * These functions are utilities that don't fit into any of the other catego-
 * ries, but do serve a purpose. In the case of regp_free() that is even an
 * essential task.
 */

void
regp_reset_session(RegP *p)
{
    p->session.sequence = 0u;
}

void
regp_free(RegP *p, RPFrame *f)
{
    if (f != NULL) {
        p->alloc->free(p->alloc->driver, f);
    }
}

RPRange
regp_range_intersection(const RPRange *a, const RPRange *b)
{
    RPRange rv = { .address = 0u, .size = 0u };
    const uint32_t start = address_max(range_start(a), range_start(b));
    const uint32_t end   = address_min(range_end(a),   range_end(b));

    if (end >= start) {
        rv.address = start;
        rv.size = end - start + 1;
    }

    return rv;
}

RPRange
regp_frame_intersection(const RPFrame *f, const RPRange *r)
{
    const RPRange a = {
        .address = f->header.address,
        .size = f->header.blocksize
    };

    return regp_range_intersection(&a, r);
}

bool
regp_empty_intersection(const RPRange *intersect)
{
    return (intersect->size > 0);
}
