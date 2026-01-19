#!/bin/sh

# Copyright (c) 2024-2026 ufw workers, All rights reserved.
#
# Redistribution  and use  in source  and binary  forms, with  or without
# modification, are permitted provided  that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions  in  binary  form  must  reproduce  the  above
#    copyright notice,  this list  of conditions and  the following
#    disclaimer  in   the  documentation  and/or   other  materials
#    provided with the distribution.
#
# THIS  SOFTWARE  IS  PROVIDED  "AS   IS"  AND  ANY  EXPRESS  OR  IMPLIED
# WARRANTIES, INCLUDING,  BUT NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
# NO EVENT SHALL THE AUTHOR OR  CONTRIBUTORS OF THE PROJECT BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL,  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS  OF USE, DATA, OR PROFITS;  OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED  AND ON  ANY THEORY OF  LIABILITY, WHETHER  IN CONTRACT,
# STRICT LIABILITY,  OR TORT (INCLUDING NEGLIGENCE  OR OTHERWISE) ARISING
# IN ANY  WAY OUT OF  THE USE  OF THIS SOFTWARE,  EVEN IF ADVISED  OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Commentary:
#
# This is an experimental test-suite  runner that emits TAP output, implemented
# for  POSIX  shells.  It  supports deterministic  and  non-deterministic  test
# suites, prerequisites, as well as setup and teardown phases.
#
# The main driver  is the "tap" function. Deterministic test  suites should use
# the "plan"  function at the  beginning of their  execution. Non-deterministic
# test-suites should call  the "noplan" function at the very  end of their exe-
# cution.
#
# The "tap" function works like this:
#
#   tap [OPTION(s)...] <TEST-RUNNER> <TEST-TITLE>
#
# The TEST-RUNNER  and TEST-TITLE arguments  are non-optional. The latter  is a
# string, that  should describe  the test  that is executed,  short and  to the
# point. The  TEST-RUNNER argument is  something executable: Often  a function,
# but external programs and scripts also work. It is called with zero arguments
# and its return-code (success or failure) determine the result of the test.
#
# Available options of "tap":
#
#   -p <PREREQUISITE-RUNNER>
#         This option takes something executable, and if this executions
#         indicates failure, it causes the test to be skipped. Successful
#         execution causes the test to be carried out.
#
#   -s <SETUP-RUNNER>
#         This option takes something executable, and runs it just before
#         a test is executed. If a prerequisite runner failed, this option
#         is ignored. The runner should return success. Failure will emit
#         a warning, and fail the test without running it.
#
#   -t <TEARDOWN-RUNNER>
#         This option is similar to "-s", only that the runner gets executed
#         just AFTER a test is executed. Returning an error from this runner
#         will invalidate the test.
#
# There are also two pre-commands for "tap", that modify a test's behaviour:
#
#   SKIP tap ...
#         This marks a test as skipped and sets its result to success. Any
#         runner options (-p, -s, and -t) will have no effect and the test
#         runner itself is not executed either.
#
#   TODO tap ...
#         This marks a test as known-to-fail. The execution of all runners
#         is carried out as normal, but TAP harnesses will not fail a suite
#         with a failing test that is marked TODO.
#
# Example:
#
# plan 1
#
# setup () {
#     printf '# I am a setup procedure!\n'
# }
#
# teardown () {
#     printf '# I am a teardown procedure!\n'
# }
#
# prereq () {
#     printf '# I am a prerequisite that always succeeds!\n'
# }
#
# the_test () {
#     printf '# I am a test that always succeeds!\n'
# }
#
# tap -s setup -t teardown -p prereq the_test 'Example test passes'
#
# Output:
#
# 1..1
# # I am a prerequisite that always succeeds!
# # I am a setup procedure!
# # I am a test that always succeeds!
# # I am a teardown procedure!
# ok 1 - Example test passes

__test_n__=0
__suffix__=''

plan () {
    printf '1..%d\n' "$1"
}

noplan () {
    printf '1..%d\n' "$__test_n__"
}

SKIP () {
    __suffix__=SKIP
    "$@"
    _rc="$?"
    __suffix__=''
    return "$_rc"
}

TODO () {
    __suffix__=TODO
    "$@"
    _rc="$?"
    __suffix__=''
    return "$_rc"
}

tap () {
    TAP_RESULT='0'
    _prereq=''
    _setup=''
    _teardown=''

    while getopts p:s:t: _opt; do
        case "$_opt" in
        p) _prereq="$OPTARG" ;;
        s) _setup="$OPTARG" ;;
        t) _teardown="$OPTARG" ;;
        *) printf '# tap: Unknown option "-%s".\n' "$_opt"
           return 1 ;;
        esac
    done
    shift $(( OPTIND - 1 ))

    if [ "$#" != 2 ]; then
        printf '# usage: tap [OPTION(s)...] RUNNER TITLE\n'
        return 1
    fi

    _prereq=${_prereq:-true}
    _runner="$1"
    _title="$2"
    _suffix=''
    _result='-init-'
    __test_n__=$(( __test_n__ + 1 ))

    if [ "$__suffix__" = SKIP ] || ! "$_prereq"; then
        _result='ok'
        _suffix='SKIP'
    else
        _suffix="$__suffix__"

        if [ -n "$_setup" ] && ! "$_setup"; then
            printf '# Setup (%s) returned failure!\n' "$_setup"
            printf '# This fails the test without running it!\n'
            TAP_RESULT='1'
        fi

        if [ "$TAP_RESULT" -eq 0 ]; then
            # This is a little complex due to the fact that POSIX shell does
            # not have $pipestatus like zsh or $PIPESTATUS like bash do. This
            # basically hands around the information via additional file
            # descriptors.
            (((((exec 3>&- 4>&-; "$_runner" 2>&1);
                printf '%s\n' "$?" >&3) |
                   sed -e 's,^,# ,' >&4) 3>&1) |
                 (read rc; exit "$rc")) 4>&1
            TAP_RESULT="$?"
            if [ -n "$_teardown" ] && ! "$_teardown"; then
                # A failing teardown runner fails the test even if the test
                # runner itself indicated success.
                printf '# Teardown (%s) returned failure!\n' "$_teardown"
                if [ "$TAP_RESULT" -eq 0 ]; then
                    printf '# This fails the test even though it succeeded'
                    printf ' in itself!\n'
                fi
                TAP_RESULT='1'
            fi
        fi

        if [ "$TAP_RESULT" -eq 0 ]; then
            _result='ok'
        else
            _result='not ok'
        fi
    fi

    printf '%s %s - %s%s\n' "$_result"    \
                            "$__test_n__" \
                            "$_title"     \
                            "${_suffix:+ # $_suffix}"
}
