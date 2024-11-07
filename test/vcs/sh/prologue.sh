#!/bin/sh

. "${MICROFRAMEWORK_ROOT:-${TOPLEVEL:-.}/../..}"/bin/tap.sh
. "${MICROFRAMEWORK_ROOT:-${TOPLEVEL:-.}/../..}"/vcs-integration/git.sh

calculate_plan () {
    REPLY=0
    for data in "$@"; do
        while read line; do
            case "$line" in
            '')   : ;;
            '#'*) : ;;
            *) REPLY=$(( REPLY + 1 )) ;;
            esac
        done < "$data"
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
