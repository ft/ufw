#!/bin/sh

cd .. || exit 1
if [ "$keep" -eq 0 ]; then
    rm -Rf specimen
fi
exit 0
