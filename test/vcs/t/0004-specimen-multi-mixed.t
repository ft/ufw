#!/bin/sh

# Copyright (c) 2024-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

TOPLEVEL="$PWD"
LIB="$TOPLEVEL"/sh
DATA="$TOPLEVEL"/data

name='specimen-multi-mixed'
SPECIMEN="$TOPLEVEL/catalogue/${name}.tar.gz"

VERSIONS='e21d10a v1.0.0 pfx/v1.0.0 4c33abf v2.0.0 d1ba1bc pfx/v2.0.0'
VERSIONS="$VERSIONS "'098d055 pfx/v3.0.0 pfx/v3.1.0 v10.0.0'
SCHEMES='default prefix'
PREFIX='pfx/'

. "$LIB"/prologue.sh
. "$LIB"/generate-states.sh

calculate_plan "$@"
plan "$REPLY"

for state in "$@"; do
    version="${state%:*}"
    file="${state#*:}"
    printf '# Run tests at version %s with %s\n' "$version" "${file##*/}"
    case "$file" in
    *-prefix[.-]*) __GIT_VERSION_PREFIX__="$PREFIX" ;;
    *)             __GIT_VERSION_PREFIX__="" ;;
    esac
    setup_repository "$version" "${file##*/}"
    git_populate
    run_with_data "$file"
done

. "$LIB"/epilogue.sh

