/*
 * Copyright (c) 2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file ex-regp-parse-frame.c
 * @brief Example application exercising regp's frame parser
 *
 * This program reads up to a MiB from stdin. If after a MiB, there is still
 * data left, the programs exits. Otherwise it feeds the data into the frame
 * parser of the register-protocol, printing a result at the end.
 *
 * This program can be used to fuzz the parser using AFL++.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ufw/compat/errno.h>
#include <ufw/compiler.h>
#include <ufw/hexdump.h>

#include "../src/register-protocol.c"

#define BUFFER_SIZE (1024ul * 1024ul)
unsigned char input[BUFFER_SIZE + sizeof(RPFrame)];

int
main(UNUSED int argc, UNUSED char *argv[])
{
    unsigned char *start = input + sizeof(RPFrame);
    size_t offset = 0u;
    bool saweof = false;

    for (;;) {
        void *pos = start + offset;
        if (offset == BUFFER_SIZE) {
            unsigned char dummy;
            const ssize_t dummyrc = read(STDIN_FILENO, &dummy, 1u);
            if (dummyrc == 0u) {
                saweof = true;
            } else {
                (void)fprintf(stderr, "stdin: More data available\n");
            }
            break;
        }
        if (offset > BUFFER_SIZE) {
            break;
        }
        const size_t rest = BUFFER_SIZE - offset;
        const ssize_t rc = read(STDIN_FILENO, pos, rest);
        if (rc < 0) {
            if (errno == -EAGAIN) {
                continue;
            }
            (void)fprintf(stderr, "error,read: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }
        if (rc == 0) {
            saweof = true;
            break;
        }
        (void)printf("stdin: Read %zd bytes.\n", rc);
        offset += rc;
    }

    if (saweof == false) {
        (void)fprintf(stderr, "More than %zu bytes input, not processing.\n",
                      BUFFER_SIZE);
        return EXIT_FAILURE;
    }

    ByteBuffer raw = BYTE_BUFFER_INIT(input, BUFFER_SIZE,
                                        sizeof(RPFrame) + offset,
                                        sizeof(RPFrame));
    RPFrame *frame = (void*)input;
    int rc = parse_frame(&raw);
    bool trydecode = true;

    printf("parse_frame: Return Value: %d\n", rc);

    if (rc < 0) {
        printf("error: %s\n", strerror(-rc));
        switch (-rc) {
        case EBUSY:
            printf("error: Allocation of a frame buffer failed.\n");
            break;
        case ENOMEM:
            printf("error: Frame buffer was too small to store incoming frame.\n");
            break;
        case EBADMSG:
            printf("error: Parsing the header failed.\n");
            trydecode = false;
            break;
        case EILSEQ:
            printf("error: Verifying the header checksum failed.\n");
            trydecode = false;
            break;
        case EFAULT:
            printf("error: Payload size is implausible, considering block size in header.\n");
            break;
        case EPROTO:
            printf("error: Verifying the payload checksum failed.\n");
            break;
        default:
            printf("Unknown error condition.\n");
            break;
        }
        if (trydecode == false) {
            return EXIT_FAILURE;
        }
    }

    printf(".header.version   = %d\n", frame->header.version);
    printf(".header.type      = %d\n", frame->header.type);
    printf(".header.meta      = 0x%1x\n", frame->header.meta.raw);
    printf(".header.options   = 0x%1x\n", frame->header.options);
    printf(".header.sequence  = 0x%04"PRIx16"\n", frame->header.sequence);
    printf(".header.address   = 0x%08"PRIx32"\n", frame->header.address);
    printf(".header.blocksize = 0x%08"PRIx32"\n", frame->header.blocksize);
    printf(".header.hdcrc     = 0x%04"PRIx16"\n", frame->header.hdcrc);
    printf(".header.plcrc     = 0x%04"PRIx16"\n", frame->header.plcrc);
    printf(".payload.size     = 0x%08zx\n", frame->payload.size);

    if (frame->payload.size > 0) {
        hexdump_stdout(frame->payload.data,
                       frame->payload.size, 0u);
    }

    return EXIT_SUCCESS;
}
