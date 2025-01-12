#!/bin/sh

esc=''

label () {
    printf '%s' "${esc}[36m"
    printf "$@"
    printf '%s' "${esc}[39m"
}

output=''
while getopts o: _opt; do
    case "$_opt" in
    o) output="$OPTARG" ;;
    *) printf 'Unknown option "-%s".\n' "$_opt"
       exit 1 ;;
    esac
done
shift $(( OPTIND - 1 ))

mode="${1:-run}"

t="$PWD"/test/module
b="$t"/build/zephyr/native-sim/mini-zephyr-fw/host/debug
zephyr="$t"/zephyr
zinclude="$zephyr"/include
zsoc="$zephyr"/soc/native/inf_clock
zboard="$zephyr"/boards/native/native_sim
zgenerated="$b"/zephyr/include/generated
zufw="$b"/modules/ufw/include

tidy_clang_tidy () {
    # Suppress processing messages
    sed '/^\[[0-9][0-9]*\/[0-9][0-9]*\] Processing file.*\.$/d
         /^[0-9][0-9]* warning\(s\|\) generated.$/d
         /^[0-9][0-9]* error\(s\|\) generated.$/d
         /^[0-9][0-9]* warning\(s\|\) and [0-9][0-9]* error\(s\|\) generated.$/d'
}

make_error () {
    # Any warning: or error: level message makes this program return failure.
    program='BEGIN { rc=0 }
             END { exit rc }
             /warning:/{ rc=1 }
             /error:/{   rc=1 }
             { print }'
    if [ -n "$output" ]; then
        awk "$program" > "$output"
        return "$?"
    else
        awk "$program"
        return "$?"
    fi
}

set -- src/*/*.[ch]          \
       src/*.[ch]            \
       include/*/*/*.[ch]    \
       include/*/*.[ch]

ensure () {
    set --                              \
        "$zgenerated"/zephyr/autoconf.h \
        "$zinclude"                     \
        "$zgenerated"                   \
        "$zsoc"                         \
        "$zboard"                       \
        "$zufw"
    erc=0
    for dep in "$@"; do
        case "$dep" in
        *.h)
            if ! [ -f "$dep" ]; then
                printf 'Required file missing: %s\n' "$dep"
                erc=1
            fi
            ;;
        *)
            if ! [ -d "$dep" ]; then
                printf 'Required directory missing: %s\n' "$dep"
                erc=1
            fi
            ;;
        esac
    done
    if [ "$erc" -eq 0 ]; then
        label 'Running clang-tidy against codebase...\n'
        if [ -n "$output" ]; then
            printf 'Output is logged to "%s".\n' "$output"
        fi
    fi
    return "$erc"
}

rc=0
case "$mode" in
run)
    ensure && clang-tidy --use-color "$@" --                      \
                         -imacros "$zgenerated"/zephyr/autoconf.h \
                         -isystem "$zinclude"                     \
                         -isystem "$zgenerated"                   \
                         -isystem "$zsoc"                         \
                         -isystem "$zboard"                       \
                         -I"$zufw"                                \
                         -Iinclude                                \
                         -DSYSTEM_ENDIANNESS_LITTLE               \
                         2>&1 | tidy_clang_tidy | make_error
    rc="$?"
    ;;
list)
    for file in "$@"; do
        printf '%s\n' "$file"
    done
    ;;
count)
    printf 'Clang-Tidy setup covers %d file(s).\n' "$#"
    ;;
*)
    printf 'Unknown mode "%s"!\n' "$mode"
    rc=1
esac

exit "$rc"
