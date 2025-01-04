# Copyright (c) 2024-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

# Generate a list of state files, that are of interest and exist.
set --
for v in $VERSIONS; do
    for d in 0 1; do
        version_to_state "$v" "$d"
        for scheme in ${SCHEMES:-none}; do
            file="$REPLY"
            if [ "$scheme" != none ]; then
                file="${file%.*}-${scheme}.${file##*.}"
            fi
            if [ -f "${DATA}/$file" ]; then
                set -- "${v}:${DATA}/$file" "$@"
            elif [ -n "$SHOW_CANDIDATES" ]; then
                printf '# Candidate: "%s"\n' "$file"
            fi
        done
    done
done
