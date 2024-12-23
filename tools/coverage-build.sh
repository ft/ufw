#!/bin/sh

out='build-coverage'

if [ -d "$out" ]; then
    rm -Rf  "$out"
fi

if [ -e "$out" ]; then
    printf 'FATAL: %s exists, giving up.\n' "$out"
    exit 1
fi

fail () {
    printf 'FATAL: Stage %s did not succeed.\n' "$1"
    exit 1
}

mmh -l -P -d "$out" prepare || fail prepare
mmh -l -P -d "$out" run cmake/native/ufw/gnu/debug/ninja \
    ++ -DUFW_ENABLE_COVERAGE=ON || fail build
gcovr --config .gcovr.conf || fail report
exit 0
