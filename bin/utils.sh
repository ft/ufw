#!/bin/sh

# Copyright (c) 2024-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if [ "$__UFW_SH_UTILS_LOADED__" = 1 ]; then
    return 0
fi
__UFW_SH_UTILS_LOADED__=1

is_opt () {
    case "$1" in
    --) return 1 ;;
    -*) return 0 ;;
    *) return 1 ;;
    esac
}
