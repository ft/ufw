#!/bin/sh

# This checks if all version entries in CHANGES are sound. The entry header
# look like this:
#
#   * v5.6.0 → v6.0.0 (unreleased):
#   * v5.5.0 → v5.6.0 (released 2024-05-28):
#   * v5.4.0 → v5.5.0 (released 2024-04-19):
#   * v5.3.1 → v5.4.0 (released 2024-03-26):
#
# The versions must thread correctly. For instance, in the first entry "v5.6.0"
# is the first version named. This *must* be the second version named in the
# second entry.
#
# The date named in "(released ...)" must match "\d+-\d+-\d+", and it must match
# the date associated with the second version named in the entry.
#
# Since this is part of release-tests, the top-level release tag and the local
# master branch must point to the same commit.

printf 'Checking state of CHANGES file...\n'

return_value=0
verbose=0
entry=0

while getopts v _opt; do
    case "$_opt" in
    v) verbose=1 ;;
    *) printf 'Unknown option "-%s".\n' "$_opt"
       exit 1 ;;
    esac
done
shift $(( OPTIND - 1 ))

vprintf () {
    [ "$verbose" -eq 0 ] && return 0
    printf "$@"
}

tag_date () {
    REPLY=$(git tag --format='%(taggerdate:format:%Y-%m-%d)' -l "$1")
}

tag_exists () {
    git rev-parse "$1" > /dev/null 2>&1
}

check_connect () {
    con="$1"
    cur="$2"
    [ -z "$con" ] && return 0
    if [ "$con" != "$cur" ]; then
        printf 'CHANGES(%d): Connecting tag is not equal: "%s" != "%s"\n' \
               "$entry" "$con" "$cur"
        return 1
    fi
    return 0
}

match_date () {
    expr "$1" : '[0-9][0-9]*-[0-9][0-9]*-[0-9][0-9]*' > /dev/null
}

check_entry () {
    entry=$(( entry + 1 ))
    p="$1"
    c="$2"
    d="$3"
    if [ "$verbose" -ne 0 ]; then
        cat <<EOF
Checking CHANGES entry ($entry)...
  previous version: $p
   current version: $c
      release date: $d
EOF
    fi

    if [ "$p" = scratch ]; then
        :
    elif ! tag_exists "$p"; then
        printf 'CHANGES(%d): Previous version does not exist: "%s"\n' \
               "$entry" "$p"
        exit 1
    else
        vprintf '  Previous release tag exists.\n'
    fi

    if ! tag_exists "$c"; then
        printf 'CHANGES(%d): Current version does not exist: "%s"\n' \
               "$entry" "$c"
        if [ "$entry" -gt 1 ]; then
            exit 1
        else
            return_value=1
        fi
    else
        vprintf '  Current release tag exists.\n'
        if [ "$entry" -eq 1 ]; then
            master=$(git rev-parse master)
            tag=$(git rev-parse "$c")
            if [ "$master" != "$tag" ]; then
                printf 'CHANGES(%d): Release tag does not match master!\n' \
                       "$entry"
                printf '  master: %s\n' "$master"
                printf '     tag: %s (%s)\n' "$master" "$c"
                exit 1
            else
                vprintf '  Release tag matches master.\n'
            fi
        fi
    fi

    if ! match_date "$d"; then
        printf 'CHANGES(%d): Release date does not match date pattern: "%s"\n' \
               "$entry" "$d"
        if [ "$d" = unreleased ]; then
            return_value=1
        else
            exit 1
        fi
    else
        vprintf '  Current release date has proper form.\n'
    fi
    tag_date "$c"
    if [ -n "$REPLY" ] && [ "$REPLY" != "$d" ]; then
        printf 'CHANGES(%d): Release date does not match tag date: "%s" != "%s"\n' \
               "$entry" "$d" "$REPLY"
        exit 1
    else
        if [ -n "$REPLY" ]; then
            vprintf '  Current release date matches entry.\n'
        elif [ -z "$REPLY" ] && [ "$d" = unreleased ]; then
            vprintf '  Unreleased version entry accordingly does not have a tag.\n'
        else
            printf 'CHANGES(%d): Tag date and release date differ (tag: "%s", release: "%s")\n' \
                   "$entry" "$REPLY" "$d"
        fi
    fi
    return 0
}

releases=$(sed -n '/^\* /p' < CHANGES | \
               sed 's,^\* *,,
                    s, *→ *, ,
                    s, *(released *, ,
                    s, *(, ,
                    s,): *$,,')

oifs="$IFS"
IFS='
'
set -- $releases
IFS="$oifs"

connect=''
for release in "$@"; do
    previous="${release%% *}"
    rest="${release#* }"
    current="${rest%% *}"
    rest="${rest#* }"
    date="${rest%% *}"
    check_connect "$connect" "$current"        || exit 1
    check_entry "$previous" "$current" "$date" || exit 1
    connect="$previous"
done

exit "$return_value"
