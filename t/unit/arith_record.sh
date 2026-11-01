#!/bin/sh
# Regenerate arith.golden by running bash 5.3 in the oracle container:
#   sh t/unit/arith_record.sh     (from WSL, or anywhere with docker)
set -e
cd "$(dirname "$0")"
docker run --rm -i --network none -e LANG=C -v "$PWD":/w:ro unabashed/bash53 \
    /usr/local/bin/bash --norc --noprofile /w/arith_record.bash \
    <arith.cases >arith.golden.tmp
mv arith.golden.tmp arith.golden
