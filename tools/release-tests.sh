#!/bin/sh

cleanup () {
    for file in test/module/build.log \
                test/module/build     \
                release.log           \
                release
    do
        if [ -d "$file" ]; then
            printf 'Removing directory %s...\n' "$file"
            rm -r "$file" || exit 1
        elif [ -f "$file" ]; then
            printf 'Removing file %s...\n' "$file"
            rm "$file" || exit 1
        fi
    done
}

usage () {
    cat <<EOF
usage: release-tests.sh COMMAND

COMMAND is one of:

  help      Displays this help text.
  clean     Cleans up some files generated by the process.
  check     Run checks that "run" executes before it does its job.
  run       Run the test
  quick     Like run, but with a set of minimal builds.

For releases, this should be run from a clean repository with
the necessary setup being done. The run command will test the
execution environment for checking if all prerequisites are
being met.
EOF
}

if [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

top_level_command="$1"

case "$top_level_command" in
check|run|quick)
    : "The rest of the script implements 'run'..."
    ;;
help)  usage;   exit 0 ;;
clean) cleanup; exit 0 ;;
*)     printf 'Unknown command "%s"!\n\n' "$1";
       usage;   exit 1 ;;
esac

cat <<EOF
=================================
|| ufw release test automation ||
=================================

EOF

missing_programs=''
mmh_min_major='0'
mmh_min_minor='28'
mmh_mandatory_toolchains='gnu gnu-arm-none-eabi mips-linux-gnu'

got_prg () {
    _prg="$1"
    oifs="$IFS"
    IFS=':'
    rc=1
    for _dir in $PATH; do
        if [ -x "$_dir"/"$_prg" ]; then
            rc=0
            break;
        fi
    done
    IFS="$oifs"
    return "$rc"
}

check_prg () {
    _prg="$1"
    printf 'Checking for program %s... ' "$_prg"
    if got_prg "$_prg"; then
        printf 'ok\n'
        return 0
    else
        printf 'NOT FOUND!\n'
        missing_programs="${missing_programs:+$missing_programs }$_prg"
        return 1
    fi
}

mmh_has_toolchain () {
    if [ -z "$mmh_toolchains" ]; then
        mmh_toolchains=$(mmh --query toolchains)
    fi
    for entry in $mmh_toolchains; do
        if [ "$entry" = "$1" ]; then
            return 0
        fi
    done
    return 1
}

zephyr_module_setup_ok () {
    test -e test/module/zephyr || return 1
    test -e test/module/ufw    || return 1
    return 0
}

# Check the environment for a suitable setup ##################################

check_prg awk
check_prg cmake
check_prg gzip
check_prg make
check_prg mmh
check_prg ninja
check_prg prove
check_prg qemu-system-arm
check_prg sed
check_prg tar

if [ -n "$missing_programs" ]; then
    printf '\nRequired programs missing:\n\n'
    for p in $missing_programs; do
        printf '  - %s\n' "$p"
    done
    printf '\nPlease resolve this situtation before retrying!\n\n'
    exit 1
fi

printf 'Checking mmh version...'

mmh_version=$(mmh --version                           | \
              head -n1                                | \
              grep -o '[0-9][0-9]*\(\.[0-9][0-9]*\)*' | \
              head -n1)

mmh_version_fields=$(printf '%s' "$mmh_version" | \
                     awk '{ gsub("[^.]", "");
                            print length+1; }')

if [ "$mmh_version_fields" != 2 ]; then
    printf '\n\nInvalid mmh version: "%s"\n\n' "$mmh_version"
    exit 1
fi

mmh_major="${mmh_version%.*}"
mmh_minor="${mmh_version#*.}"

if [ "$mmh_major" -lt "$mmh_min_major" ] || \
   [ "$mmh_minor" -lt "$mmh_min_minor" ]
then
    printf '\n\nMakeMeHappy installation too old.\n'
    printf 'We need at least %s.%s, but only found %s.\n' \
           "$mmh_min_major" "$mmh_min_minor" "$mmh_version"
    printf 'Please upgrade!\n\n'
    exit 1
fi

printf ' ok (%s)\n' "$mmh_version"

mmh_toolchains_ok='1'
printf 'Checking for mandatory mmh toolchains...\n'

for tc in $mmh_mandatory_toolchains; do
    printf '  ...toolchain %s... ' "$tc"
    if mmh_has_toolchain "$tc"; then
        printf 'ok\n'
    else
        printf 'missing\n'
        mmh_toolchains_ok='0'
    fi
done

if [ "$mmh_toolchains_ok" -eq 0 ]; then
    printf 'Checking for mandatory mmh toolchains... INVALID\n'
    cat <<EOF

Your mmh configuration does not configure all mandatory toolchains,
that we would really like to run release tests with. The gnu tool-
chain is pretty easy to fulfil. So is the bare metal arm cross tool-
chain. With the latter, we run the test suite through qemu. The mips
toolchain is important since it allows running the test-suite, again
through qemu, with a big-endian architecture.

On debian, here are a couple of packages that might be required for
the mips toolchain to work.

  - gcc-mips-linux-gnu
  - g++-mips-linux-gnu
  - g++-12-mips-linux-gnu
  - gcc-12-multilib-mips-linux-gnu
  - g++-12-multilib-mips-linux-gnu
  - g++-multilib-mips-linux-gnu
  - gcc-multilib-mips-linux-gnu

Good luck!

Obviously more toolchains are always good to throw against the code-
base. Consider things like clang, arm-zephyr-eabi, or ti-c2000, for
extra bonus points!

EOF
    exit 1
else
    printf 'Checking for mandatory mmh toolchains... ok\n'
fi

printf 'Checking zephyr-module test setup...'
if zephyr_module_setup_ok; then
    printf ' ok\n'
else
    printf ' BROKEN\n'
    cat <<EOF

Please run the setup script in test/module to enable the zephyr
module build test-suite.

EOF
    exit 1
fi

if [ "$top_level_command" = check ]; then
    printf '\nEnvironment looks suitable.\n'
    exit 0
fi

printf '\nEnvironment looks suitable. Let'\''s go!\n\n'

# Running the actual tests now. Rather easy once everything is in place. ######

set -- --directory release --log-to-file --show-phases --succeed

if [ "$top_level_command" = quick ]; then
    set -- "$@" --config tools/quick.yaml
fi

mmh "$@"
mmh --quiet result --short release.log || exit 1
(cd test/module && prove -v -c run)    || exit 1
(cd test/vcs    && prove -c t/*.t)     || exit 1

cat <<EOF

Looks like everthing passed. Nice.

Do not forget to update include/ufw/meta.h and update CHANGES before
tagging the system for release!

EOF

exit 0