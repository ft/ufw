#!/bin/sh

# Copyright (c) 2024-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

# This runs a single build of ufw  with coverage tracking enabled. The build is
# done with  a native toolchain, and  exacutes the test-suite, of  course. This
# run will log the coverage data to  the build-tree. The script then runs gcovr
# to produce an html coverage report into: build-coverage/index.html
#
# Without arguments, this  uses mmh to perform  a fresh build each  time in the
# build-coverage subdirectory  (this can be  adjusted with the -o  option). You
# can also perform  an incremental build using the -r  option, which reruns the
# build and test stages instead of running a new build from nothing.

out='build-coverage'
rerun=0

while getopts o:r _opt; do
    case "$_opt" in
    o) out="$OPTARG" ;;
    r) rerun=1 ;;
    *) printf 'Unknown option "-%s".\n' "$_opt"
       exit 1 ;;
    esac
done
shift $(( OPTIND - 1 ))

if [ "$rerun" -eq 0 ] && [ -d "$out" ]; then
    rm -Rf  "$out"
fi

if [ "$rerun" -eq 0 ] && [ -e "$out" ]; then
    printf 'FATAL: %s exists, giving up.\n' "$out"
    exit 1
fi

fail () {
    printf 'FATAL: Stage %s did not succeed.\n' "$1"
    exit 1
}

xninja () {
    ninja -C "$out"/build/cmake/native/ufw/gnu/debug/ninja "$@"
}

if [ "$rerun" -eq 0 ]; then
    mmh -l -P -d "$out" prepare || fail prepare
    mmh -l -P -d "$out" run cmake/native/ufw/gnu/debug/ninja \
        ++ -DUFW_ENABLE_COVERAGE=ON                          \
           -DGENERATE_API_DOCUMENTATION=ON                   \
        || fail build
else
    xninja all  || fail rebuild
    xninja test || fail retest
fi

gcovr --config .gcovr.conf || fail report
exit 0
