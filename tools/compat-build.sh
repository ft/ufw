#!/bin/sh

# Copyright (c) 2024-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if [ "$#" -ne 0 ] && [ "$#" -ne 1 ]; then
    cat <<EOF
usage: compliance-build.sh [PREVIOUS-STATE]
EOF
    exit 1
fi

fail () {
    printf 'FATAL: Stage %s did not succeed.\n' "$1"
    exit 1
}

origin="$PWD"
work='build-compat'
name='ufw'
variant="cmake/native/${name}/gnu/debug/ninja"

if [ -d "$work" ]; then
    rm -Rf  "$work"
fi

if [ -e "$work" ]; then
    printf 'FATAL: %s exists, giving up.\n' "$work"
    exit 1
fi

log='check-compat'
output='reports'

dir_this='current'
dir_prev='previous'

state_this=$(git describe --always --abbrev=16)
if [ "$#" -eq 1 ]; then
    state_prev=$(git describe --always --abbrev=16 "$1")
else
    state_prev=$(git describe --always --abbrev=0 'HEAD^')
fi

printf 'Comparing ABI/API of %s\n  from: %s\n    to: %s\n' \
       "$name" "$state_prev" "$state_this"

mkdir "$work" || fail make-work-dir
cd "$work"

git_clone () {
    dir="$1"
    state="$2"
    printf '\nMaking %s available in %s\n' "$state" "$dir"
    git clone --quiet "$origin" "$dir" || fail make-"$dir"
    git -C "$dir" -c advice.detachedHead=false checkout "$state" \
        || fail state-"$state"
    return 0
}

build_the_thing () {
    dir="$1"
    printf '\nBuilding %s in %s...\n' "$variant" "$dir"
    (cd "$dir"                                                             && \
         mmh -d ci -q prepare                                              && \
         mmh -d ci -l -S run "$variant" ++ -DUFW_ENABLE_DYNAMIC_LIBRARY=ON && \
         mmh -d ci -q result -s ci.log) || fail build-"$dir"
}

git_clone "$dir_prev" "$state_prev"
git_clone "$dir_this" "$state_this"

build_the_thing "$dir_prev"
build_the_thing "$dir_this"

lib="libufw-full.so"
subdir="ci/build/$variant"

dump_filter () {
    sed '/^ctags:/d'
}

dump_the_thing () {
    dir="$1"
    state="$2"
    printf '\nDumping ABI data for %s in %s\n' "$state" "$dir"
    cd "$dir"
    abi-dumper "$subdir"/code-under-test/"$lib" \
               -o ABI.dump                      \
               -lver "$state"                   \
               -public-headers include/ || fail abi-"$state"
    cd ..
}

{ dump_the_thing "$dir_prev" "$state_prev"; } 2>&1 | dump_filter
{ dump_the_thing "$dir_this" "$state_this"; } 2>&1 | dump_filter

printf '\nRunning ABI/API checker...\n'
exec abi-compliance-checker -l "$name"               \
                            -report-path report.html \
                            -old previous/ABI.dump   \
                            -new current/ABI.dump
