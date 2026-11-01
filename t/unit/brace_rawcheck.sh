#!/bin/sh
# Check t/unit/brace_raw.golden against bash 5.3 in the oracle container
# $ sh t/unit/brace_rawcheck.sh   (in WSL, from the repository root)
#
# For each case, bash expands the word, then expands the expected raw
# words with brace expansion turned off, and the two must agree.
#
# This is free and unencumbered software released into the public domain.
docker run --rm -i --network none -e LANG=C unabashed/bash53 \
    /usr/local/bin/bash --norc --noprofile -c '
set -f
x=X a=ABC b=B xa=1 xb=2
n=0 bad=0 have=0
check() {
    [ $have = 1 ] || return 0
    n=$((n+1))
    got=$(eval "set -- $w"; printf "[%s]" "$@"; printf "%d" $#)
    want=$(set +B; eval "set -- $exp"; printf "[%s]" "$@"; printf "%d" $#)
    if [ "$got" != "$want" ]; then
        bad=$((bad+1))
        printf "MISMATCH %s\n  bash:     %s\n  expected: %s\n" "$w" "$got" "$want"
    fi
}
while IFS= read -r line; do
    case $line in
    "> "*) check; w=${line#> }; exp=; have=1;;
    "= "*) exp="$exp ${line#= }";;
    "=")   ;;  # an empty raw word is a null field, which bash drops
    esac
done
check
echo "$n cases, $bad mismatched"
[ $bad = 0 ]' <t/unit/brace_raw.golden
