# Record t/unit/test.golden from bash 5.3 in the oracle container.
#   $ wsl -d Debian -- sh /mnt/c/Users/wellons/src/unabashed/t/unit/test_record.sh
# This is free and unencumbered software released into the public domain.
set -e
dir=$(cd "$(dirname "$0")" && pwd)
docker run --rm -i --network none -v "$dir:/w:ro" unabashed/bash53 \
    /usr/local/bin/bash --norc --noprofile /w/test_oracle.sh /w/test.cases \
    >"$dir/test.golden.tmp"
mv "$dir/test.golden.tmp" "$dir/test.golden"
