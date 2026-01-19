#!/bin/sh

# Copyright (c) 2025-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

bd='build-apidoc'
log="${bd}.log"

usage () {
    cat <<EOF

usage:   ./tools/apidoc-build.sh [OPTION(s)...]

Available options:

  -h       Display this help text
  -r       Only create $bd if it does not exist yet
  -e       Enable WARN_AS_ERROR in doxygen execution
  -o DIR   Use DIR instead of $bd for building documentation

Use -r after initial builds for much increased generation speeds.

EOF
}

esc=''

label () {
    printf '%s' "${esc}[36m"
    printf "$@"
    printf '%s' "${esc}[39m"
}

rerun=0
allow_warnings=1

while getopts eho:r _opt; do
    case "$_opt" in
    e) allow_warnings=0 ;;
    h) usage; exit 0 ;;
    o) bd="$OPTARG" ;;
    r) rerun=1 ;;
    *) printf 'Unknown option "-%s".\n' "$_opt"
       exit 1 ;;
    esac
done
shift $(( OPTIND - 1 ))

label 'Building API Documentation...\n'

if [ -e api ] && (! [ -h api ] ); then
    printf '"api" exists but is not a symlink! Giving up.\n'
    exit 1
fi

if [ -d "$bd" ] && [ "$rerun" -ne 0 ] && [ -f "$bd"/build.ninja ]; then
    :
elif [ -e "$bd" ]; then
    rm -Rf "$bd" || exit 1
fi

mkdir -p "$bd" || exit 1
if ! [ -f "$bd"/build.ninja ]; then
    printf 'Generating build environment in %s...\n' "$bd"
    cmake -GNinja -B "$bd" -S . -DGENERATE_API_DOCUMENTATION=True \
          2>&1 > "$log" || {
        printf 'CMake failed. Check %s.\n' "$log"
        exit 1
    }
fi

if [ "$allow_warnings" -ne 0 ]; then
    DOXYGEN_WARN_AS_ERROR=NO
else
    DOXYGEN_WARN_AS_ERROR=YES
fi
export DOXYGEN_WARN_AS_ERROR
printf 'Documentation warnings are treated as errors: %s\n' \
       "$DOXYGEN_WARN_AS_ERROR"

ninja -C "$bd" ufw-api-documentation || exit 1

if [ -e api ] && (! [ -h api ] ); then
    printf ''
fi

[ -e api ] && rm api
ln -sf "$bd"/doc/api-documentation/html api

printf 'API documentation available in "api/index.html".\n'
exit 0
