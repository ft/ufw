#!/bin/sh

# Copyright (c) 2024-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

. "${MICROFRAMEWORK_ROOT:-${TOPLEVEL:-.}/../..}"/bin/tap.sh
. "${MICROFRAMEWORK_ROOT:-${TOPLEVEL:-.}/../..}"/vcs-integration/git.sh

calculate_plan () {
    REPLY=0
    for data in "$@"; do
        case "$data" in
        *:*) file="${data#*:}" ;;
        *)   file="$data"      ;;
        esac
        while read line; do
            case "$line" in
            '')   : ;;
            '#'*) : ;;
            *) REPLY=$(( REPLY + 1 )) ;;
            esac
        done < "$file"
    done
}

data_file_line_test () {
    actual=$(  eval 'printf '\''%s'\'' "${'"$variable"'}"')
    if [ "$actual" != "$value" ]; then
        printf 'Actually: %s='\''%s'\'' != '\''%s'\''\n' \
               "$variable" "$actual" "$value"
        return 1
    fi
    return 0
}

run_with_data () {
    _data_file="$1"
    while IFS='' read -r line; do
        case "$line" in
        '')   : ;;
        '#'*) : ;;
        *) variable="${line%%=*}"
           value="${line#*=}"
           tap data_file_line_test "Test if $variable matches '$value'"
           ;;
        esac
    done < "$_data_file"
}

specimen_cleanup () {
    if [ "$keep" -eq 0 ]; then
        rm -Rf specimen
    fi
}

test_abort () {
    specimen_cleanup
    exit 1
}

version_to_state () {
    __version="$1"
    __isclean="$2"
    case "$__version" in
    pfx/*) REPLY="p${__version#pfx/v}" ;;
    v*)    REPLY="$__version"          ;;
    *)     REPLY="g$__version"         ;;
    esac
    REPLY=$( printf '%s' "$REPLY" | sed -e 's,\.,,g' )
    if [ "$__isclean" -ne 0 ]; then
        REPLY="${name}-${REPLY}-clean.state"
    else
        REPLY="${name}-${REPLY}-dirty.state"
    fi
}

make_dirty () {
    mv .gitignore foo || test_abort
    head -n 5 foo > .gitignore || test_abort
    rm foo || test_abort
    return 0
}

git_checkout () {
    command git checkout "$@" > /dev/null 2>&1
}

git_clean () {
    command git clean "$@" > /dev/null
}

git_reset () {
    command git reset "$@" > /dev/null
}

setup_repository () {
    # Be extra careful, since some of the clean up commands in here are
    # devastating, when called in a repository they are not meant for.
    _top_actual=$(git rev-parse --show-toplevel)
    _top_expect="$TOPLEVEL/specimen"
    if [ "$_top_actual" != "$_top_expect" ]; then
        printf '# setup_repository: Broken repository: "%s" != "%s"\n' \
               "$_top_actual" "$_top_expect"
        test_abort
    fi
    git_reset --hard  || test_abort
    git_clean -xdf    || test_abort
    git_checkout "$1" || test_abort
    case "$2" in
    *-dirty[.-]*) make_dirty ;;
    esac
    return 0
}

if [ -z "$SPECIMEN" ]; then
    printf '# prologue: SPECIMEN is empty! Giving up.\n'
    exit 1
fi

if [ -d specimen ]; then
    printf '# Specimen directory exists! Using it.\n'
    keep=1
elif [ -e specimen ]; then
    printf '# Specimen directory in unexpected state. Giving up.\n'
    exit 1
else
    tar xf "$SPECIMEN" || exit 1
    keep=0
fi

cd specimen || exit 1
