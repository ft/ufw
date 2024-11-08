#!/bin/sh

TOPLEVEL="$PWD"
LIB="$TOPLEVEL"/sh

name='specimen-single-top-vtag'
DATA_clean="$TOPLEVEL/data/${name}-clean.state"
DATA_dirty="$TOPLEVEL/data/${name}-dirty.state"
SPECIMEN="$TOPLEVEL/catalogue/${name}.tar.gz"

. "$LIB"/prologue.sh

calculate_plan "$DATA_clean" "$DATA_dirty"
plan "$REPLY"

printf '# Run tests with clean repository\n'
git_populate
run_with_data "$DATA_clean"

make_dirty

printf '# Run tests with dirty repository\n'
git_populate
run_with_data "$DATA_dirty"

. "$LIB"/epilogue.sh
