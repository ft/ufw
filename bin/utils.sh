#!/bin/sh

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
