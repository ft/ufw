/*
 * Copyright (c) 2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file ex-regp-parse-frame.c
 * @brief Example application exercising the RFC1055 parser
 *
 * This reads a SLIP (rfc1055) frame from stdin and writes the raw frame to
 * stdout. Any extra bytes from stdin are ignored. If no complete frame is
 * found on stdin, EXIT_FAILURE is returned; EXIT_SUCCESS otherwise.
 *
 * When used without arguments the classic, end-of-frame only format, is used.
 * With -s the the with-start-of-frame variant is used. The same can also be
 * achieved by setting the UFW_RFC1055_WITH_SOF environment variable to "1".
 *
 * This program can be used to fuzz the parser using AFL++.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ufw/compat/errno.h>
#include <ufw/compiler.h>
#include <ufw/endpoints.h>
#include <ufw/rfc1055.h>

int
main(int argc, char *argv[])
{
    RFC1055Context ctx;
    Source source;
    Sink sink;
    int fd_stdin  = STDIN_FILENO;
    int fd_stdout = STDOUT_FILENO;
    int code = EXIT_SUCCESS;
    bool usesof = false;

    int opt;
    while ((opt = getopt(argc, argv, "s")) != -1) {
        switch (opt) {
        case 's':
            usesof = true;
            break;
        default:
            printf("Unknown option: %c\n", opt);
            return EXIT_FAILURE;
        }
    }

    const char *envsof = getenv("UFW_RFC1055_WITH_SOF");
    if (envsof != NULL && strcmp(envsof, "1") == 0) {
        usesof = true;
    }
    rfc1055_context_init(&ctx, usesof ? RFC1055_WITH_SOF : RFC1055_DEFAULT);
    source_from_filedesc(&source, &fd_stdin);
    sink_to_filedesc(&sink, &fd_stdout);

    for (;;) {
        const int rc = rfc1055_decode(&ctx, &source, &sink);
        if (rc > 0) {
            break;
        }
        if (rc < 0) {
            (void)fprintf(stderr, "# errno: %d (%s)\n", -rc, strerror(-rc));
            if (rc == -ENODATA) {
                code = EXIT_FAILURE;
                break;
            }
        }
    }

    return code;
}
