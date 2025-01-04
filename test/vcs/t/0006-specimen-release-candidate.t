#!/bin/sh

# Copyright (c) 2024-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

TOPLEVEL="$PWD"
LIB="$TOPLEVEL"/sh
DATA="$TOPLEVEL"/data

name='specimen-release-candidate'
SPECIMEN="$TOPLEVEL/catalogue/${name}.tar.gz"

VERSIONS='main v1.0.0-rc1'
SCHEMES=''

. "$LIB"/prologue.sh
. "$LIB"/generate-states.sh

calculate_plan "$@"
plan "$REPLY"

for state in "$@"; do
    version="${state%:*}"
    file="${state#*:}"
    printf '# Run tests at version %s with %s\n' "$version" "${file##*/}"
    setup_repository "$version" "${file##*/}"
    git_populate
    run_with_data "$file"
done

. "$LIB"/epilogue.sh

