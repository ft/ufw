#!/bin/sh

# Copyright (c) 2024-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

# This runs many tests with ufw, that previously were done manually upon re-
# lease, to make sure no unforseen issues crept in. This obviously runs the
# full range of builds (which depends on mmh configuration), but also checks
# ABI/API compatibility, as well as updates to ufw/meta.h and CHANGES, among
# other tests as well.

esc=''

label () {
    printf '%s' "${esc}[36m"
    printf "$@"
    printf '%s' "${esc}[39m"
}

fail () {
    printf 'FATAL: Stage %s did not succeed.\n' "$1"
    exit 1
}

bad_stuff=''

bad () {
    printf 'WARNING: Stage %s did not succeed.\n' "$1"
    bad_stuff="${bad_stuff:+$bad_stuff }$1"
    return 0
}

cleanup () {
    for file in test/module/build.log \
                test/module/build     \
                release.log           \
                release
    do
        if [ -d "$file" ]; then
            printf 'Removing directory %s...\n' "$file"
            rm -r "$file" || fail rm-dir-"$file"
        elif [ -f "$file" ]; then
            printf 'Removing file %s...\n' "$file"
            rm "$file" || fail rm-file-"$file"
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

if [ "$#" -lt 1 ]; then
    usage
    exit 1
fi

top_level_command="$1"
shift

case "$top_level_command" in
check|run|quick)
    : "The rest of the script implements 'run'..."
    ;;
help)  usage;   exit 0 ;;
clean) cleanup; exit 0 ;;
*)     printf 'Unknown command "%s"!\n\n' "$1";
       usage;   exit 1 ;;
esac

previous_version=''
while getopts p: _opt; do
    case "$_opt" in
    p) previous_version="$OPTARG" ;;
    *) printf 'Unknown option "-%s".\n' "$_opt"
       exit 1 ;;
    esac
done
shift $(( OPTIND - 1 ))

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

check_prg abi-compliance-checker
check_prg abi-dumper
check_prg awk
check_prg cmake
check_prg ctags
check_prg date
check_prg doxygen
check_prg gcovr
check_prg git
check_prg gzip
check_prg make
check_prg mmh
check_prg ninja
check_prg perl
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
    fail mmh-version-validity
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
    fail mmh-version-requirement
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
  - g++-multilib-mips-linux-gnu
  - gcc-multilib-mips-linux-gnu

A config snippet for mmh to add this toolchains could look like this:

  toolchains:
    - name: mips-linux-gnu
      architecture: [ mips ]

Good luck!

Obviously more toolchains are always good to throw against the code-
base. Consider things like clang, arm-zephyr-eabi, or ti-c2000, for
extra bonus points!

EOF
    fail mmh-minimum-toolchains
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
    fail zephyr-module-test-setup
fi

if [ "$top_level_command" = check ]; then
    printf '\nEnvironment looks suitable.\n'
    exit 0
fi

printf '\nEnvironment looks suitable. Let'\''s go!\n'

# Running the actual tests now. Rather easy once everything is in place. ######

set -- --directory release --log-to-file --show-phases --succeed

if [ "$top_level_command" = quick ]; then
    set -- "$@" --config tools/quick.yaml
fi

label '\nRelease build...\n'
mmh "$@"
mmh --quiet result --short release.log  || bad mmh-release-build
mmh --quiet result --report release.log || bad mmh-release-warnings
label '\nZephyr module build...\n'
(cd test/module && prove -v -c run)     || bad zephyr-module-build
label '\nVCS integration tests...\n'
(cd test/vcs    && prove -c t/*.t)      || bad vcs-integration
label '\nTest coverage build...\n'
./tools/coverage-build.sh               || bad test-coverage-build

if [ -n "$previous_version" ]; then
    set -- "$previous_version"
else
    set --
fi

label '\nABI/API compatibility test build...\n'
./tools/compat-build.sh "$@"           || {
    printf 'Warning: ABI/API compatibility may be broken!\n'
    bad library-abi-api-compatibility
}

printf '\n'
./tools/check-changes.sh || bad check-changes-file

printf '\n'
./tools/check-meta-h.sh || bad check-meta-h-file

printf '\n'
./tools/check-copyright.sh || bad check-copyright

# Summarise test results ######################################################

return_value=0
if [ -n "$bad_stuff" ]; then
    return_value=1
    printf '\nEncountered bad test results:\n\n'
    for thing in $bad_stuff; do
        printf '  - %s\n' "$thing"
        case "$thing" in
        mmh-release-build)
            cat <<EOF

    The  release build  failed. This  absolutely cannot  happen. This  may also
    cause other  sub-tests to fail.  This can build  caused by build  errors or
    test suite failures. Fix this!

EOF
        ;;
        mmh-release-warnings)
            cat <<EOF

    MakeMeHappy detected compiler incidents (errors,  warnings, etc) in the log
    file of  the release build. The  library should be warning  free at release
    time. Fix this before continuing with the release process!

EOF
        ;;
        zephyr-module-build)
            cat <<EOF

    Where was  an issue in  the zephyr module build  type test. This  cannot be
    allowed in a release. Check test/module/build.log and fix the issue!

EOF
        ;;
        vcs-integration)
            cat <<EOF

    At  least on  of the  version-control integration  tests failed.  This test
    suite must  pass before release.  Run "cd  test/vcs && prove  -vc ./t/\*.t"
    find the issue and fix it!

EOF
        ;;
        test-coverage-build)
            cat <<EOF

    The test-coverage build failed. This  should not happen, unless the release
    build also failed. This basically just  adds --coverage to the compiler and
    linker flags used in the build, which GCC should support.

EOF
        ;;
        check-changes-file)
            cat <<EOF

    The CHANGES  file has issues. Maybe  the release notes for  the new release
    are missing. Maybe  there is still an "unreleased" date  in there. Or maybe
    the date for the release is incorrect. There are other possible issues. Fix
    this before pushing data to public repositories!

EOF
        ;;
        check-meta-h-file)
            cat <<EOF

    The  file include/ufw/meta.h  reflects the  release version  of ufw.  It is
    maintained manually, in order to simplify  the build process of ufw as much
    as possible.  Currently there is a  mismatch between the macros  defined in
    the file and the state reflected by  git. In some instances, the macros are
    already bumped to the intended next version tag during development. In that
    case, the issue  will be resolved when  the new tag is put  into place. But
    make sure the situation is correct before pushing to public repositories!

EOF
        ;;
        check-copyright)
            cat <<EOF

    The release-tests  detected issues with  copyright notices in at  least one
    file. All files that need it should  have a copyright notice, that is up to
    date. At  the very  latest this has  to be the  case upon  release. Earlier
    would better, of course.

EOF
        ;;
        library-abi-api-compatibility)
            cat <<EOF

    The ABI  and/or API  compatibility check  failed. It  is possible  that the
    build connected to the check failed,  or the ABI/API was extended or broken
    comparing to the last  release. This may be intended. But  you need to make
    sure, that  the release type  (major, minor)  matches the kinds  of changes
    detected in this  check. Patch releases MUST  NOT change the API  or ABI at
    all!

EOF
        ;;
        esac
    done

    cat <<EOF
Note that  among the possible failures,  only library-abi-api-compatibility can
be allowed with to  remain with a valid release. ABI and/or  API changes can be
intended with minor  or major releases. This is within  the discretion of ufw's
maintainer. Everything must be fixed before making a release public!
EOF

    if [ "$bad_stuff" = library-abi-api-compatibility ]; then
        return_value=0
        printf '\nThis is the case here!\n'
    fi
else
    printf '\nLooks like everthing passed. Nice.\n'
fi

cat <<EOF

Reports and documentation for review:

  api/index.html
  build-coverage/index.html
  build-compat/report.html

EOF

# Aaaand we are done... #######################################################

exit "$return_value"
