# Generate a list of state files, that are of interest and exist.
set --
for v in $VERSIONS; do
    for d in 0 1; do
        version_to_state "$v" "$d"
        if [ -f "${DATA}/$REPLY" ]; then
            set -- "${v}:${DATA}/$REPLY" "$@"
        fi
    done
done
