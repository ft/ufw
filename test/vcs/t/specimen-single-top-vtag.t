#!/bin/sh

TOPLEVEL="$PWD"
LIB="$TOPLEVEL"/sh

name='specimen-single-top-vtag'
DATA_clean="$TOPLEVEL/data/${name}-clean.state"
DATA_dirty="$TOPLEVEL/data/${name}-dirty.state"
SPECIMEN="$TOPLEVEL/catalogue/${name}.tar.gz"

. "$LIB"/prologue.sh

set -- "$DATA_clean" "$DATA_dirty"
calculate_plan "$@"
plan "$REPLY"

printf '# Run tests with clean repository\n'
git_populate
run_with_data "$DATA_clean"

mv README foo || exit 1
head -n 5 foo > README || exit 1
rm foo || exit 1

printf '# Run tests with dirty repository\n'
git_populate
run_with_data "$DATA_dirty"

. "$LIB"/epilogue.sh
