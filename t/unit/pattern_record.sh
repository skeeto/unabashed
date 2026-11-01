#!/bin/sh
# Record t/unit/pattern.golden from bash 5.3 in the oracle container.
#   $ wsl -d Debian -- sh /mnt/c/Users/wellons/src/unabashed/t/unit/pattern_record.sh
# This is free and unencumbered software released into the public domain.
set -e
dir=$(cd "$(dirname "$0")" && pwd)
docker run --rm -i --network none -v "$dir:/w:ro" unabashed/bash53 \
    /usr/local/bin/bash --norc --noprofile /w/pattern_oracle.bash \
    <"$dir/pattern.cases" >"$dir/pattern.golden.$$"
mv "$dir/pattern.golden.$$" "$dir/pattern.golden"
