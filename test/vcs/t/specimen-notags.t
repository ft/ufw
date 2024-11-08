#!/bin/sh

TOPLEVEL="$PWD"
LIB="$TOPLEVEL"/sh

name='specimen-notags'
DATA="$TOPLEVEL/data/${name}.state"
SPECIMEN="$TOPLEVEL/catalogue/${name}.tar.gz"

. "$LIB"/prologue.sh

calculate_plan "$DATA"
plan "$REPLY"
git_populate
run_with_data "$DATA"

. "$LIB"/epilogue.sh
