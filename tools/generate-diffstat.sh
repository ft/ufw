#!/bin/sh

# Copyright (c) 2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

prev_from_cmdline="$1"

usage () {
    cat <<EOF
usage: generate-diffstat.sh [OPTION(s)...]

Options:

  -s         Show the commit-range to use for diffstat.
  -p PREV    Use PREV as the previous version.

EOF
}

show=0
prev_from_cmdline=''
while getopts hp:s _opt; do
    case "$_opt" in
    p) prev_from_cmdline="$OPTARG" ;;
    s) show=1 ;;
    h) usage
       exit 0
       ;;
    *) printf 'Unknown option "-%s".\n' "$_opt"
       exit 1 ;;
    esac
done
shift $(( OPTIND - 1 ))

if [ "$#" -gt 0 ]; then
    usage
    exit 1
fi

indent_for_changes () {
    sed -e 's@^ *@    @'
}

previous_release () {
    if [ -n "$prev_from_cmdline" ]; then
        printf '%s\n' "$prev_from_cmdline"
    else
        git describe --always --abbrev=0
    fi
}

diffstat_since () {
    previous="$1"
    git diff --stat-width=72 "$1".. -- ':!CHANGES'
}

if [ "$show" -ne 0 ]; then
    printf '%s..HEAD\n' "$(previous_release)"
else
    diffstat_since "$(previous_release)" | indent_for_changes
fi

exit 0
