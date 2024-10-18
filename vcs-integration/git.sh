#!/bin/sh

# This library has these public functions:
#
#   git_populate: Fetches information from git database and populates the
#       following parameters, if it can:
#
#         __GIT_LOADED__
#         __GIT_BRANCH__
#         __GIT_COMMITDATE__
#         __GIT_COMMITDATE_UNIX__
#         __GIT_DESCRIPTION__
#         __GIT_DIRTY__
#         __GIT_HASH__
#         __GIT_HASH_FULL__
#         __GIT_INCREMENT__
#         __GIT_MAJOR__
#         __GIT_MINOR__
#         __GIT_PATCHLEVEL__
#         __GIT_VERSION__
#         __GIT_VERSION_AVAILABLE__
#         __GIT_VERSION_IS_CANDIDATE__
#         __GIT_CANDIDATE_LEVEL__
#         __GIT_VERSION_IS_PRE_RELEASE__
#         __GIT_PRE_RELEASE_LEVEL__
#         __GIT_IS_CLEAN_RELEASE_BUILD__
#
# The code uses the variable __GIT_VERSION_PREFIX__ as a string to match and
# strip off of version tags. This allows users to use foobar/v1.0.0 to result
# in v1.0.0 when __GIT_VERSION_PREFIX__='foobar/'.
#
#   git_amend_versions: Overrides the MAJOR_VERSION, MINOR_VERSION and
#       PATCHLEVEL shell parameters using their git counterparts.
#
#   git_got_info: Returns true if the information gathered by git_populate
#       looks reasonable.
#
#   git_exact_version: Returns true if the currently checked out commits points
#       to an annotated tag in git that matches "v[0-9]*". The value of the pa-
#       rameter __GIT_VERSION_PREFIX__ is honoured by this function.
#
#   git_detached_head: Returns true, if __GIT_BRANCH__ suggests, the git
#       repository is in DetachedHead state.
#
#   git_dirty: Returns true, if there are uncommitted changes in tracked files
#       in the git repository.
#
# Example:
#
#     git_populate
#     git_got_info && git_amend_versions

__GIT_LOADED__=1

_git_version_ () {
    REPLY="$(git describe --always \
                          --match "${__GIT_VERSION_PREFIX__}"'v[0-9]*' \
                          --abbrev=0)"
    REPLY="${REPLY#${__GIT_VERSION_PREFIX__}}"
    case "$REPLY" in
    v*) : ;;
    *) REPLY=noversion
    esac
}

_git_increment_ () {
    REPLY="$(git rev-list --count "$1"..HEAD)"
}

_git_commitdate_ () {
    REPLY="$(git show -s --date=format:%Y-%m-%d --format=%cd)"
}

_git_commitdate_unix_ () {
    REPLY="$(git show -s --format=%ct)"
}

_git_hash_ () {
    REPLY="$(git rev-parse --short=12 HEAD)"
}

_git_hash_full_ () {
    REPLY="$(git rev-parse HEAD)"
}

_git_branch_ () {
    branch_="$(git rev-parse --abbrev-ref=strict HEAD)"
    case "$branch_" in
    HEAD) branch_="DetachedHead" ;;
    *) : ;;
    esac
    REPLY="$(printf '%s' "$branch_" | sed -e 's,[/ ][/ ]*,-,g')"
}

_git_description_ () {
    REPLY="$(git show -s --pretty="tformat:%h (%s, %ai" | \
             sed -e 's~ [012][0-9]:[0-5][0-9]:[0-5][0-9] [-+][0-9][0-9][0-9][0-9]$~)~')"
}

_git_in_worktree_ () {
    git rev-parse --is-inside-work-tree > /dev/null;
}

git_detached_head () {
    test "$__GIT_BRANCH__" = "DetachedHead"
}

git_dirty () {
    git update-index -q --refresh
    if [ -n "$(git diff-index --name-only HEAD --)" ]; then
        # Yup, it's dirty.
        return 0
    fi
    return 1
}

git_got_info () {
    test "x$__GIT_MAJOR__" = x && return 1
    test "x$__GIT_MINOR__" = x && return 1
    test "x$__GIT_PATCHLEVEL__" = x && return 1
    return 0
}

git_amend_versions () {
    MAJOR_VERSION="$__GIT_MAJOR__"
    MINOR_VERSION="$__GIT_MINOR__"
    PATCHLEVEL="$__GIT_PATCHLEVEL__"
}

git_exact_version () {
    REPLY="$(git describe --match "${__GIT_VERSION_PREFIX__}"'v[0-9]*' \
                          --exact-match 2> /dev/null)"
    [ "x$REPLY" != x ]
}

git_is_clean_full_release () {
    test "$__GIT_DIRTY__" = 0 || return 1
    test "$__GIT_INCREMENT__" = 0 || return 1
    test "$__GIT_VERSION_IS_CANDIDATE__" = 0 || return 1
    test "$__GIT_VERSION_IS_PRE_RELEASE__" = 0 || return 1
    return 0
}

git_str_is_int () {
    expr match "$1" '^[0-9][0-9]*$' > /dev/null 2>&1
}

git_populate () {
    __GIT_BRANCH__=''
    __GIT_COMMITDATE__=''
    __GIT_COMMITDATE_UNIX__=''
    __GIT_DESCRIPTION__=''
    __GIT_DIRTY__=0
    __GIT_HASH__=''
    __GIT_HASH_FULL__=''
    __GIT_INCREMENT__=0
    __GIT_MAJOR__=0
    __GIT_MINOR__=0
    __GIT_PATCHLEVEL__=0
    __GIT_VERSION__=''
    __GIT_VERSION_AVAILABLE__=0
    __GIT_VERSION_IS_CANDIDATE__=0
    __GIT_CANDIDATE_LEVEL__=0
    __GIT_VERSION_IS_PRE_RELEASE__=0
    __GIT_PRE_RELEASE_LEVEL__=0
    __GIT_IS_CLEAN_RELEASE_BUILD__=0
    if _git_in_worktree_; then
        _git_branch_
        __GIT_BRANCH__="$REPLY"

        _git_commitdate_
        __GIT_COMMITDATE__="$REPLY"

        _git_commitdate_unix_
        __GIT_COMMITDATE_UNIX__="$REPLY"

        _git_description_
        __GIT_DESCRIPTION__="$REPLY"

        if git_dirty; then
            __GIT_DIRTY__=1
        else
            __GIT_DIRTY__=0
        fi

        _git_hash_
        __GIT_HASH__="$REPLY"

        _git_hash_full_
        __GIT_HASH_FULL__="$REPLY"

        _git_version_
        __GIT_VERSION__="$REPLY"
        case "$__GIT_VERSION__" in
        noversion)
            __GIT_VERSION__="NoVersion-g$__GIT_HASH__"
            ;;
        *)  # Seems like we found a reasonable tag. Disect major/minor/pl from
            # it, using POSIX parameter expansions. Maximum information level
            # from __GIT_VERSION__:
            #
            #   v12.23.42-rc666.b
            #
            # ...where the -rc666.b is pretty much free form, parsed into
            # __GIT_EXTRA__. This is the format, that should be used for
            # version tags.
            __GIT_MAJOR__="${__GIT_VERSION__#v}"
            __GIT_MAJOR__="${__GIT_MAJOR__%%.*}"
            __GIT_MINOR__="${__GIT_VERSION__#*.}"
            __GIT_MINOR__="${__GIT_MINOR__%%.*}"
            __GIT_PATCHLEVEL__="${__GIT_VERSION__#*.}"
            __GIT_PATCHLEVEL__="${__GIT_PATCHLEVEL__#*.}"
            __GIT_PATCHLEVEL__="${__GIT_PATCHLEVEL__%%-*}"
            __GIT_EXTRA__="${__GIT_VERSION__##*-}"

            if test "$__GIT_EXTRA__" = "$__GIT_VERSION__"; then
                __GIT_EXTRA__=''
            fi

            __GIT_VERSION_IS_CANDIDATE__=0
            __GIT_VERSION_IS_PRE_RELEASE__=0
            case "$__GIT_EXTRA__" in
            rc*) __GIT_VERSION_IS_CANDIDATE__=1
                 n=${__GIT_EXTRA__#rc}
                 if git_str_is_int "$n"; then
                     __GIT_CANDIDATE_LEVEL__="$n"
                 else
                     __GIT_CANDIDATE_LEVEL__="0"
                 fi
                 ;;
            pre*) __GIT_VERSION_IS_PRE_RELEASE__=1
                  n=${__GIT_EXTRA__#pre}
                  if git_str_is_int "$n"; then
                      __GIT_PRE_RELEASE_LEVEL__="$n"
                  else
                      __GIT_PRE_RELEASE_LEVEL__="0"
                  fi
                  ;;
            esac

            # Determine increment from tag to head:
            _git_increment_ "${__GIT_VERSION_PREFIX__}$__GIT_VERSION__"
            __GIT_INCREMENT__="$REPLY"

            # Now extend __GIT_VERSION__, if we're not exactly on a release
            # tag:
            if ! git_exact_version; then
                __GIT_VERSION__="${__GIT_VERSION__}-${__GIT_INCREMENT__}"
                __GIT_VERSION__="${__GIT_VERSION__}-g${__GIT_HASH__}"
            fi
            ;;
        esac
        __GIT_VERSION__="${__GIT_VERSION__#v}"
        if test "$__GIT_DIRTY__" = 1; then
            __GIT_VERSION__="${__GIT_VERSION__}-dirty"
        fi
        if git_is_clean_full_release; then
            __GIT_IS_CLEAN_RELEASE_BUILD__=1
        else
            __GIT_IS_CLEAN_RELEASE_BUILD__=0
        fi
    else
        __GIT_BRANCH__=""
        __GIT_DESCRIPTION__=""
        __GIT_DIRTY__=""
        __GIT_HASH__=""
        __GIT_HASH_FULL__=""
        __GIT_INCREMENT__=""
        __GIT_MAJOR__=""
        __GIT_MINOR__=""
        __GIT_PATCHLEVEL__=""
        __GIT_VERSION__=""
        __GIT_IS_CLEAN_RELEASE_BUILD__=""
        __GIT_VERSION_IS_CANDIDATE__=""
        __GIT_VERSION_IS_PRE_RELEASE__=""
    fi

    if git_got_info; then
        __GIT_VERSION_AVAILABLE__=1
    fi
}
