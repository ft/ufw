#!/bin/sh

# Copyright (c) 2024-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

# This checks the contents of "include/ufw/meta.h". The macros defined therein
# must reflect the major, minor, and patchlevel fields of the closest tag visi-
# ble. In connection with check-changes.sh, this makes sure that all fields are
# correct at release-time. Both of these are combined in release-tests.sh.

esc=''

label () {
    printf '%s' "${esc}[36m"
    printf "$@"
    printf '%s' "${esc}[39m"
}

release="$(git describe --always)"
major="${release#v}"
major="${major%%.*}"
minor="${release#*.}"
minor="${minor%%.*}"
patch="${release#*.}"
patch="${patch#*.}"
patch="${patch%%[!0-9]*}"

label 'Checking meta.h for major(%s), minor(%s), patchlevel(%s)...\n' \
      "$major" "$minor" "$patch"

rematch () {
    expr "$1" : "$2" > /dev/null
}

version () {
    line="$1"
    part="$2"
    if rematch "$line" '^#define *UFW_LIBRARY_'"$part"' *[0-9][0-9]*u$'; then
        REPLY="${line##* }"
        REPLY="${REPLY%u}"
        return 0
    fi
    return 1
}

return_value=0
mismatch () {
    part="$1"
    git="$2"
    metafile="$3"
    if [ "$metafile" != "$git" ]; then
        return_value=1
        printf 'Version mismatch (%s): metafile(%s) != git(%s)\n' \
               "$part" "$metafile" "$git"
        return_value=1
    fi
}

count () {
    name="$1"
    count="$2"
    if [ "$count" -ne 1 ]; then
        printf 'Definition for UFW_LIBRARY_%s was found %d times!\n' \
               "$name" "$count"
        return_value=1
    fi
}

maj=0
min=0
pat=0
while read line; do
    if version "$line" MAJOR; then
        maj=$(( maj + 1 ))
        mismatch major "$major" "$REPLY"
    elif version "$line" MINOR; then
        min=$(( min + 1 ))
        mismatch minor "$minor" "$REPLY"
    elif version "$line" PATCH; then
        pat=$(( pat + 1 ))
        mismatch patchlevel "$patch" "$REPLY"
    fi
done < include/ufw/meta.h

count MAJOR "$maj"
count MINOR "$min"
count PATCH "$pat"

if [ "$return_value" -eq 0 ]; then
    printf 'ufw/meta.h looks consistent.\n'
fi

exit "$return_value"
