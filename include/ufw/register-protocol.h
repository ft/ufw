/*
 * Copyright (c) 2023-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_REGISTER_PROTOCOL_H_3d84e123
#define INC_UFW_REGISTER_PROTOCOL_H_3d84e123

/**
 * @addtogroup protoregp Simple Register Protocol
 *
 * Implementation of a register protocol for embedded systems
 *
 * @{
 *
 * @file ufw/register-protocol.h
 * @brief Simple Register Protocol API
 *
 * @}
 */

#include <stddef.h>
#include <stdint.h>

#include <ufw/allocator.h>
#include <ufw/bit-operations.h>
#include <ufw/endpoints.h>
#include <ufw/register-table.h>
#include <ufw/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern BlockAllocator rp_default_allocator;

#define RP_IMPLEMENTATION_VERSION 0u

#define RP_DEFAULT_BUFFER_SIZE 128ULL

#define RP_OPT_WORD_SIZE_16     BIT(0)
#define RP_OPT_WITH_HEADER_CRC  BIT(1)
#define RP_OPT_WITH_PAYLOAD_CRC BIT(2)

typedef enum RPFrameType {
    RP_FRAME_INVALID        = -1,
    RP_FRAME_READ_REQUEST   =  0,
    RP_FRAME_READ_RESPONSE  =  1,
    RP_FRAME_WRITE_REQUEST  =  2,
    RP_FRAME_WRITE_RESPONSE =  3,
    RP_FRAME_META           = 15
} RPFrameType;

typedef enum RPResponse {
    RP_RESP_ACK = 0,
    RP_RESP_EWORDSIZE,
    RP_RESP_EPAYLOADCRC,
    RP_RESP_EPAYLOADSIZE,
    RP_RESP_ERXOVERFLOW,
    RP_RESP_ETXOVERFLOW,
    RP_RESP_EBUSY,
    RP_RESP_EUNMAPPED,
    RP_RESP_EACCESS,
    RP_RESP_ERANGE,
    RP_RESP_EINVALID,
    RP_RESP_EIO
} RPResponse;

typedef enum RPMetaMeta {
    RP_META_EHEADERENC = 1,
    RP_META_EHEADERCRC,
} RPMetaMeta;

typedef struct RPBlockAccess {
    RPResponse status;
    uint32_t address;
} RPBlockAccess;

#define RPB_BLOCK_ACCESS_INIT { .status = RP_RESP_ACK, .address = 0u }

typedef RPBlockAccess (*RPBlockRead16)(uint32_t, size_t, uint16_t*);
typedef RPBlockAccess (*RPBlockWrite16)(uint32_t, size_t, const uint16_t*);
#ifdef WITH_UINT8_T
typedef RPBlockAccess (*RPBlockRead8)(uint32_t, size_t, uint8_t*);
typedef RPBlockAccess (*RPBlockWrite8)(uint32_t, size_t, const uint8_t*);

typedef struct RPMemory8 {
    RPBlockRead8 read;
    RPBlockWrite8 write;
} RPMemory8;
#endif /* WITH_UINT8_T */

typedef struct RPMemory16 {
    RPBlockRead16 read;
    RPBlockWrite16 write;
} RPMemory16;

#define RP_MEM16_NOTHING         \
    { .read  = regp_void_read16, \
      .write = regp_void_write16 }

typedef enum RPMemoryType {
    RP_MEMTYPE_8,
    RP_MEMTYPE_16
} RPMemoryType;

typedef struct RPMemory {
    RPMemoryType type;
    union {
#ifdef WITH_UINT8_T
        RPMemory8 m8;
#endif /* WITH_UINT8_T */
        RPMemory16 m16;
    } access;
} RPMemory;

#define RP_MEM_NOTHING               \
    { .type = RP_MEMTYPE_16,         \
      .access.m16 = RP_MEM16_NOTHING }

typedef struct RPSession {
    uint16_t sequence;
} RPSession;

#define RP_NEW_SESSION { .sequence = 0u }

typedef enum RPEndpointType {
    RP_EP_SERIAL,
    RP_EP_TCP
} RPEndpointType;

typedef struct RPEndpoint {
    RPEndpointType type;
    Source source;
    Sink sink;
} RPEndpoint;

#define RP_ENDPOINT_NULL        \
    { .type = RP_EP_TCP,        \
      .source = source_empty,   \
      .sink = sink_null         }

typedef struct RegP {
    RPMemory memory;
    RPSession session;
    RPEndpoint ep;
    BlockAllocator *alloc;
} RegP;

#define RP_NEW_INSTANCE              \
    { .memory = RP_MEM_NOTHING,      \
      .session = RP_NEW_SESSION,     \
      .ep = RP_ENDPOINT_NULL,        \
      .alloc = &rp_default_allocator }

typedef struct RPFrame {
    /* This structured data is huge (well, comparatively). I wonder, since
     * we're saving the raw frame anyway, and parsing this protocol is nearly
     * trivial, if we can just implement accessors that read the desired data
     * from the raw frame buffer. */
    struct {
        uint_least8_t version;
        RPFrameType type;
        uint_least8_t options;
        union {
            RPResponse response;
            RPMetaMeta meta;
            unsigned int raw;
        } meta;
        uint16_t sequence;
        uint32_t address;
        uint32_t blocksize;
        uint16_t hdcrc;
        uint16_t plcrc;
    } header;
    struct {
        size_t size;
        void *data;
    } payload;
    struct {
        void *memory;
        size_t size;
    } raw;
} RPFrame;

typedef struct RPMaybeFrame {
    struct {
        int id;
        size_t framesize;
    } error;
    RPFrame *frame;
} RPMaybeFrame;

typedef struct RPRange {
    uint32_t address;
    size_t size;
} RPRange;

/* Void Memory Implementation */
RPBlockAccess regp_void_read16(uint32_t address, size_t bsize, uint16_t *buf);
RPBlockAccess regp_void_write16(uint32_t address, size_t bsize,
                                const uint16_t *buf);

/* Initialisation */
void regp_init(RegP *p);
#ifdef WITH_UINT8_T
void regp_use_memory8(RegP *p, RPBlockRead8 read, RPBlockWrite8 write);
#endif /* WITH_UINT8_T */
void regp_use_memory16(RegP *p, RPBlockRead16 read, RPBlockWrite16 write);
void regp_use_channel(RegP *p, RPEndpointType type, Source source, Sink sink);
void regp_use_allocator(RegP *p, BlockAllocator *alloc);

/* Request API */
int regp_req_read8(RegP *p, uint32_t address, size_t n);
int regp_req_read16(RegP *p, uint32_t address, size_t n);
#ifdef WITH_UINT8_T
int regp_req_write8(RegP *p, uint32_t address, size_t n, const uint8_t *buf);
#endif /* WITH_UINT8_T */
int regp_req_write16(RegP *p, uint32_t address, size_t n, const uint16_t *buf);

/* Response API */
int regp_resp_ack(RegP *p, const RPFrame *f, const void *pl, size_t n);
int regp_resp_ewordsize(RegP *p, const RPFrame *f);
int regp_resp_epayloadcrc(RegP *p, const RPFrame *f);
int regp_resp_epayloadsize(RegP *p, const RPFrame *f);
int regp_resp_erxoverflow(RegP *p, const RPFrame *f, uint32_t size);
int regp_resp_etxoverflow(RegP *p, const RPFrame *f, uint32_t size);
int regp_resp_ebusy(RegP *p, const RPFrame *f);
int regp_resp_eunmapped(RegP *p, const RPFrame *f, uint32_t address);
int regp_resp_eaccess(RegP *p, const RPFrame *f, uint32_t address);
int regp_resp_erange(RegP *p, const RPFrame *f, uint32_t address);
int regp_resp_einvalid(RegP *p, const RPFrame *f, uint32_t address);
int regp_resp_eio(RegP *p, const RPFrame *f);
int regp_resp_meta(RegP *p, uint_least8_t meta);

/* Processing API */
int regp_recv(RegP *p, RPMaybeFrame *mf);
int regp_process(RegP *p, const RPMaybeFrame *mf);

/* Matching API */
bool regp_is_valid(const RPFrame *f);
bool regp_is_request(const RPFrame *f);
bool regp_is_response(const RPFrame *f);
bool regp_is_read_request(const RPFrame *f);
bool regp_is_write_request(const RPFrame *f);
bool regp_is_read_response(const RPFrame *f);
bool regp_is_write_response(const RPFrame *f);
bool regp_is_meta_message(const RPFrame *f);
bool regp_is_16bitsem(const RPFrame *f);
bool regp_has_hdcrc(const RPFrame *f);
bool regp_has_plcrc(const RPFrame *f);

/* Miscellaneous API */
void regp_free(RegP *p, RPFrame *f);
void regp_reset_session(RegP *p);
bool regp_empty_intersection(const RPRange *intersect);
RPRange regp_range_intersection(const RPRange *a, const RPRange *b);
RPRange regp_frame_intersection(const RPFrame *f, const RPRange *r);

static inline RPBlockAccess
regaccess2blockaccess(const RegisterAccess access)
{
    RPBlockAccess rc = { .address = access.address };

    switch (access.code) {
    case REG_ACCESS_SUCCESS:       rc.status = RP_RESP_ACK;       break;
    case REG_ACCESS_UNINITIALISED: /* FALLTHROUGH */
    case REG_ACCESS_NOENTRY:       rc.status = RP_RESP_EUNMAPPED; break;
    case REG_ACCESS_RANGE:         rc.status = RP_RESP_ERANGE;    break;
    case REG_ACCESS_INVALID:       rc.status = RP_RESP_EINVALID;  break;
    case REG_ACCESS_READONLY:      rc.status = RP_RESP_EACCESS;   break;
    case REG_ACCESS_FAILURE:       /* FALLTHROUGH */
    case REG_ACCESS_IO_ERROR:      /* FALLTHROUGH */
    default:                       rc.status = RP_RESP_EIO;       break;
    }

    return rc;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_REGISTER_PROTOCOL_H_3d84e123 */
