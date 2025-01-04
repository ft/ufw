#!/bin/sh

# Copyright (c) 2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

# This script has heuristics for checking copyright notices as the one right
# above this comment in the ufw codebase. The program takes one optional
# argument, which is the mode of operation:
#
#    check            Check copyright notices in git-tracked files.
#    actice           List files that are considered for the check.
#    inactive         List files that are NOT considered for the check.
#    update           Update existing copyright notices.
#    add              Add copyright notices to files that do not have one.
#
# If called without an argument, "check" is used.
#
# The "update" and "add" modes are best-effort and might not be perfect. You
# always need to carefully check, if the actions taken worked or not.

mode="${1:-check}"
update=0
add=0
case "$mode" in
update) update=1
        mode=check
        ;;
add) add=1
     mode=check
     ;;
esac
match='Copyright.*ufw workers.*reserved.'
old_and_new='Copyright.*\(micro framework workers\|ufw\).*reserved.'
old_and_new_perl='Copyright.*(micro framework workers|ufw).*reserved.'

# This function is called for each file name tracked by ufw's git repository.
# For each file it gets two arguments: The directory name the file lives in and
# the name of the file itself. Based on these two names, it decides whether or
# not the script checks for a copyright header with the current year as the end
# of the period. When extending this, try to keep patterns open, so that new
# files added get picked up rather than not.
needs_copyright () {
    __directory__="$1"
    __filename__="$2"

    case "$__directory__" in
    .)
        case "$__filename__" in
        LICENCE)        return 0 ;;
        CMakeLists.txt) return 0 ;;
        module.yaml)    return 0 ;;
        *)              return 1 ;;
        esac
        ;;
    cmake)
        case "$__filename__" in
        README.*) return 1 ;;
        *)        return 0 ;;
        esac
        ;;
    cmake/kconfig)
        case "$__filename__" in
        README) return 1 ;;
        *)      return 0 ;;
        esac
        ;;
    cmake/modules)
        case "$__filename__" in
        pandocology.cmake) return 1 ;;
        *)                 return 0 ;;
        esac
        ;;
    doc|doc/*)
        case "$__filename__" in
        CMakeLists.txt) return 0 ;;
        *)              return 1 ;;
        esac
        ;;
    fuzz/*)
        case "$__filename__" in
        *.dat) return 1 ;;
        *)     return 0 ;;
        esac
        ;;
    test|test/*)
        case "$__filename__" in
        README)           return 1 ;;
        specimen-*.state) return 1 ;;
        *.tar.gz)         return 1 ;;
        .git*)            return 1 ;;
        *)                return 0 ;;
        esac
        ;;
    vcs-integration)
        case "$__filename__" in
        *.in)      return 1 ;;
        *.example) return 1 ;;
        *)         return 0 ;;
        esac
        ;;
    zephyr|zephyr/*)
        case "$__filename__" in
        vendor-prefixes.txt) return 1 ;;
        *)                   return 0 ;;
        esac
        ;;
    # Everything else is active.
    *) return 0 ;;
    esac
}

git_start_date () {
    __filename__="$1"
    REPLY=$(git log --diff-filter=A  \
                    --follow         \
                    --date=format:%Y \
                    --format=%cd     \
                    --  "$__filename__" | tail -n1)
}

tag_year () {
    REPLY=$(git tag --format='%(taggerdate:format:%Y)' -l "$1")
}

get_year () {
    REPLY=$(printf '%s\n' "$1" | perl -lne 'if (/(\d+-\d+)/) { print $1 }
                                            elsif (/(\d+)/)  { print $1 }')
}

make_notice () {
    from="$1"
    to="$2"
    if [ "$from" != "$to" ]; then
        REPLY="Copyright (c) ${from}-${to} ufw workers, All rights reserved."
    else
        REPLY="Copyright (c) ${from} ufw workers, All rights reserved."
    fi
}

make_full_notice () {
    _type="$1"
    _copy="$2"
    _terms='Terms for redistribution and use can be found in LICENCE.'
    case "$_type" in
    shell) REPLY="# $_copy
#
# $_terms
" ;;
    cmake|kconfig|shell-include|yaml) REPLY="# $_copy
#
# $_terms

" ;;
    c-*|device-tree) REPLY="/*
 * $_copy
 *
 * $_terms
 */

" ;;
    *) printf 'BUG: Unsupported type: "%s"\n' "$_type"
       exit 1 ;;
    esac
}

update_file () {
    _file="$1"
    _new="$2"
    perl -i -pe 's%'"$old_and_new_perl"'%'"$_new"'%' "$_file"
}

detect_file_type () {
    _first=$(head -n1 "$1")
    if [ "$_first" = '#!/bin/sh' ]; then
       REPLY=shell
       return 0
    fi
    case "$1" in
    CMakeLists.txt|*/CMakeLists.txt)  REPLY=cmake ;;
    *.cmake)                          REPLY=cmake ;;
    *.conf)                           REPLY=kconfig ;;
    Kconfig|*/Kconfig)                REPLY=kconfig ;;
    *.h)                              REPLY=c-header ;;
    *.h.in)                           REPLY=c-header-template ;;
    *.c)                              REPLY=c-source ;;
    *.sh)                             REPLY=shell-include ;;
    *.overlay)                        REPLY=device-tree ;;
    *.yaml|*.yml)                     REPLY=yaml ;;
    *)                                REPLY=UNKNOWN ;;
    esac
    return 0
}

add_notice () {
    _file="$1"
    _new="$2"
    _offset=1
    detect_file_type "$_file"
    if [ "$REPLY" = UNKNOWN ]; then
        printf '%s: Unknown file format. Cannot add notice.\n' "$_file"
        return 1
    fi
    if [ "$REPLY" = shell ]; then
        _offset=2
    fi
    make_full_notice "$REPLY" "$_new"
    if [ "$_offset" -gt 1 ]; then
        REPLY="
$REPLY"
    fi
    printf '%s: Trying to add copyright notice. You better check this!\n' \
           "$_file"
    perl -i -pe 'print "'"$REPLY"'" if $. == '"$_offset" "$_file"
}

check_file () {
    _end="$1"
    _file="$2"
    _rc=0
    if ! grep -q "$old_and_new" "$_file"; then
        printf '%s: No copyright notice!\n' "$_file"
        if [ "$add" -ne 0 ]; then
            git_start_date "$_file"
            _start="$REPLY"
            make_notice "$_start" "$_end"
            new_notice="$REPLY"
            add_notice "$_file" "$new_notice"
        fi
        return 1
    fi
    oifs="$IFS"
    IFS='
'
    set -- $(grep "$match" "$_file")
    IFS="$oifs"
    if [ "$#" -eq 0 ]; then
        printf '%s: Deprecated copyright notice!\n' "$_file"
        oifs="$IFS"
        IFS='
'
        set -- $(grep "$old_and_new" "$_file")
        IFS="$oifs"
        _rc=1
    fi
    if [ "$#" -gt 1 ]; then
        case "$_file" in
        # For these files, the first match is the one we want.
        tools/check-copyright.sh)     : ;;
        tools/make-binary-format.scm) : ;;
        *) printf '%s: Uses %d copyright notices\n' "$_file" "$#"
           return 1
           ;;
        esac
    fi
    git_start_date "$_file"
    _start="$REPLY"
    notice="$1"
    get_year "$notice"
    actual="$REPLY"
    if [ "$_start" = "$_end" ]; then
        expect="$_start"
    else
        expect="${_start}-${_end}"
    fi
    if [ "$actual" != "$expect" ]; then
        printf '%s: Expected "%s"\n' "$_file" "$expect"
        printf '%s:   Actual "%s"\n' "$_file" "$actual"
        make_notice "$_start" "$_end"
        new_notice="$REPLY"
        if [ "$update" -ne 0 ]; then
            update_file "$_file" "$new_notice"
        fi
        _rc=1
    fi
    return "$_rc"
}

current=$(git describe --always)
latest=$(git describe --always --abbrev=0)

if [ "$mode" = check ]; then
    printf 'Checking copyright notices in codebase...\n'
    if [ "$current" = "$latest" ]; then
        tag_year "$current"
        endyear="$REPLY"
        printf 'Using end-year from tag (%s): %s\n' "$latest" "$endyear"
    else
        endyear=$(date +'%Y')
        printf 'Using end-year based on today: %s\n' "$endyear"
    fi
    printf '\n'
fi

rc=0
set -- $(git ls-files)
for file in "$@"; do
    case "$file" in
    */*) home="${file%/*}"
         name="${file##*/}"
         ;;
    *)   home="."
         name="$file"
         ;;
    esac
    case "$mode" in
    active)
        if needs_copyright "$home" "$name"; then
            printf '%s\n' "$file"
        fi
        ;;
    inactive)
        if ! needs_copyright "$home" "$name"; then
            printf '%s\n' "$file"
        fi
        ;;
    check)
        if needs_copyright "$home" "$name"; then
            check_file "$endyear" "$file" || rc=1
        fi
        ;;
    esac
done

exit "$rc"
