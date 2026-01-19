#!/bin/sh

# Copyright (c) 2025-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

esc=''

label () {
    printf '%s' "${esc}[36m"
    printf "$@"
    printf '%s' "${esc}[39m"
}

bd='build-analyze'
rerun=0
allow_warnings=1
while getopts o:r _opt; do
    case "$_opt" in
    o) bd="$OPTARG" ;;
    r) rerun=1 ;;
    *) printf 'Unknown option "-%s".\n' "$_opt"
       exit 1 ;;
    esac
done
shift $(( OPTIND - 1 ))
log="$bd".log

UFW_CLANG_ANALYZER="${UFW_CLANG_ANALYZER:-analyze-build-19}"

label 'Running clang'\''s static analyzer...\n'

printf 'Logging to: %s\n' "$log"

if (! [ -d "$bd" ]) || [ "$rerun" -eq 0 ]; then
    rm -Rf "$bd"
    mkdir -p "$bd"/html
    printf 'Generating build-tree for analyzer.\n'
    cmake -S . -B "$bd"                                   \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=YES             \
          -DCMAKE_C_COMPILER=$(which clang) > "$log" 2>&1 \
        || {
        printf 'Failed to generate build-tree.\n'
        exit 1
    }
fi

printf 'Running %s...\n' "${UFW_CLANG_ANALYZER}"
rc=0
"${UFW_CLANG_ANALYZER}" --cdb "$bd"/compile_commands.json \
                        --use-analyzer $(which clang)     \
                        --analyze-headers                 \
                        --status-bugs                     \
                        --output "$bd"/html > "$log" 2>&1 \
    || rc=1

if [ "$rc" -eq 0 ]; then
    printf 'No issues found. Great!\n'
else
    __newest=$( ls -t1 "$bd"/html/*/index.html 2>/dev/null | head -n1 )
    if [ -n "$__newest" ]; then
        newest="${__newest%/index.html}"
        newest="${newest#$bd/}"
        if [ -e "$bd"/report ]; then
            rm "$bd"/report || exit 1
        fi
        if [ -d "$bd"/"$newest" ]; then
            ln -s "$newest" "$bd"/report || exit 1
            printf '\n'
            if [ "$rerun" -eq 0 ]; then
                w3m -dump "$bd"/report/index.html
            fi
            printf 'Report available in: %s\n' "$bd"/report/index.html
        fi
    fi
fi

exit "$rc"
