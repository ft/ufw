/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file rfc1055.c
 * @brief RFC1055: Transmission of IP Datagrams over Serial Lines: SLIP
 *
 * See https://www.rfc-editor.org/rfc/rfc1055.txt for details.
 */

#include <stdint.h>

#include <ufw/compat/errno.h>

#include <ufw/bit-operations.h>
#include <ufw/compiler.h>
#include <ufw/rfc1055.h>
#include <ufw/endpoints.h>

/*
 * RFC1055 Control Characters
 */

/**
 * Indicate End-of-Frame
 *
 * The End-of-Frame delimiter can also be used as a Start-of-Frame delimiter,
 * resulting in streams like this:
 *
 * ...<EOF>payload...<EOF><EOF>payload...<EOF><EOF>...
 *
 * This is what the RFC suggests as an extension, because it does not increase
 * the overhead cause by the octet-stuffing technique.
 */
#define RAW_EOF 0xc0u

/**
 * Indicate Control Character in Payload
 *
 * In order to eliminate RFC1055's control character from its arbitrary payload
 * alphabet, the RFC defines two-octet sequences to replace them in encoded
 * data. The RAW_ESC character inside such encoded data indicates that such a
 * replacement has taken place.
 *
 * This technique causes a worst-case overhead of the input data size resulting
 * in encoded frame-size: 2·n_raw + 1. (+2 in case a start-of-frame delimiter
 * is used.
 */
#define RAW_ESC 0xdbu

/**
 * Escaped End-of-Frame
 *
 * When ESC_EOF appears in encoded payload after RAW_ESC, that means the
 * original data contained a single RAW_EOF.
 */
#define ESC_EOF 0xdcu

/**
 * Escaped Escape
 *
 * When ESC_ESC appears in encoded payload after RAW_ESC, that means the
 * original data contained a single RAW_ESC.
 */
#define ESC_ESC 0xddu

#define MAYBE_RETURN(EXPR)                      \
    do {                                        \
        int ufw_return_code = EXPR;             \
        if (ufw_return_code < 0) {              \
            return ufw_return_code;             \
        }                                       \
    } while (0)

void
rfc1055_context_init(RFC1055Context *ctx, uint32_t flags)
{
    ctx->flags = flags;
    ctx->state = BIT_ISSET(ctx->flags, RFC1055_WITH_SOF)
        ? RFC1055_SEARCH_FOR_START
        : RFC1055_NORMAL;
}

static inline int
rfc1055_open(const RFC1055Context *ctx, Sink *sink)
{
    if (BIT_ISSET(ctx->flags, RFC1055_WITH_SOF)) {
        const int rc = sink_put_octet(sink, RAW_EOF);
        return rc < 0 ? rc : 0;
    }

    return 0;
}

static inline int
rfc1055_close(Sink *sink)
{
    const int rc = sink_put_octet(sink, RAW_EOF);
    return rc < 0 ? rc : 0;
}

static const unsigned char esc_esc[2] = { RAW_ESC, ESC_ESC };
static const unsigned char esc_eof[2] = { RAW_ESC, ESC_EOF };

static inline int
rfc1055_encode_octet(Sink *sink, unsigned char data)
{
    int rc;

    switch (data) {
    case RAW_ESC: rc = sink_put_chunk(sink, esc_esc, sizeof(esc_esc)); break;
    case RAW_EOF: rc = sink_put_chunk(sink, esc_eof, sizeof(esc_eof)); break;
    default:      rc = sink_put_octet(sink, data);                     break;
    }

    return rc;
}

static inline int
rfc1055_decode_octet(Source *source, unsigned char *data)
{
    int rv = 1;
    unsigned char first;
    /* Guarantee that the data return value is initialised, no matter the
     * behaviour of the source implementation. This is important with the
     * EBADMSG return code, which this SLIP decoder uses. While unlikely that
     * the source returns that particular error code, it is not impossible.
     * Modern compilers even warn about this. */
    *data = 0u;
    MAYBE_RETURN(source_get_octet(source, &first));
    switch (first) {
    case RAW_ESC:
    {
        rv++;
        unsigned char second;
        MAYBE_RETURN(source_get_octet(source, &second));
        switch (second) {
        case ESC_EOF: *data = RAW_EOF; break;
        case ESC_ESC: *data = RAW_ESC; break;
        default:      *data = second;  return -EILSEQ;
        }
    }
        break;
    case RAW_EOF:
        return 0;
    default:
        *data = first;
        break;
    }

    return rv;
}

int
rfc1055_encode(const RFC1055Context *ctx, Source *source, Sink *sink)
{
    MAYBE_RETURN(rfc1055_open(ctx, sink));
    for (;;) {
        unsigned char data;
        const int get = source_get_octet(source, &data);
        if (get < 0) {
            return get;
        } else if (get == 0) {
            break;
        }
        MAYBE_RETURN(rfc1055_encode_octet(sink, data));
    }
    return rfc1055_close(sink);
}

static inline int
transition(Source *source)
{
    unsigned char data;
    MAYBE_RETURN(source_get_octet(source, &data));
    return (data == RAW_EOF) ? 1 : 0;
}

int
rfc1055_decode(RFC1055Context *ctx, Source *source, Sink *sink)
{
    unsigned char data = 0u;

    for (;;) {
        switch (ctx->state) {
        case RFC1055_SEARCH_FOR_START: {
            const int rc = transition(source);
            if (rc < 0) {
                return 0;
            } else if (rc == 1) {
                ctx->state = RFC1055_NORMAL;
            }
            break;
        }
        case RFC1055_SEARCH_FOR_END: {
            const int rc = transition(source);
            if (rc < 0) {
                return 0;
            } else if (rc == 1) {
                ctx->state =
                    BIT_ISSET(ctx->flags, RFC1055_WITH_SOF)
                    ? RFC1055_SEARCH_FOR_START
                    : RFC1055_NORMAL;
            }
            break;
        }
        case RFC1055_NORMAL: /* FALLTHROUGH */
        default: {
            const int rc = rfc1055_decode_octet(source, &data);
            if (rc == -EILSEQ) {
                if (BIT_ISSET(ctx->flags, RFC1055_WITH_SOF)) {
                    ctx->state = (data == RAW_EOF)
                        ? RFC1055_SEARCH_FOR_START
                        : RFC1055_SEARCH_FOR_END;
                } else {
                    ctx->state = (data == RAW_EOF)
                        ? RFC1055_NORMAL
                        : RFC1055_SEARCH_FOR_END;
                }
                return rc;
            } else if (rc < 0) {
                /* Forward source error to caller. */
                return rc;
            } else if ((rc == 0)) {
                /* End-of-Frame found */
                if (BIT_ISSET(ctx->flags, RFC1055_WITH_SOF)) {
                    ctx->state = RFC1055_SEARCH_FOR_START;
                }
                return 0;
            }

            MAYBE_RETURN(sink_put_octet(sink, data));
            break; }
        }
    }

    /* NOTREACHED */
}
