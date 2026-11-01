#!/bin/sh
# Record t/unit/brace.golden from bash 5.3 in the oracle container
# $ sh t/unit/brace_gen.sh        (in WSL, from the repository root)
#
# Each line of brace_words.txt is one shell word. bash expands it with
# "set -- WORD" (the same word expansion as printf '[%s]\n' WORD, but an
# empty result stays visible) and prints each resulting field as [field].
# Globbing is off, and no word may contain an expansion other than inside
# single quotes, so the expansion is brace expansion plus quote removal.
#
# This is free and unencumbered software released into the public domain.
set -e
dir=t/unit
out=$dir/brace.golden
tmp=$out.tmp
{
    echo '# Recorded from bash 5.3 by t/unit/brace_gen.sh: do not edit.'
    echo '# "> WORD" then one "[field]" line per resulting field.'
    docker run --rm -i --network none -e LANG=C unabashed/bash53 \
        /usr/local/bin/bash --norc --noprofile -c '
set -f
while IFS= read -r w; do
    case $w in ""|"#"*) continue;; esac
    printf "> %s\n" "$w"
    eval "set -- $w"
    if [ $# -gt 0 ]; then
        printf "[%s]\n" "$@"
    fi
done' <$dir/brace_words.txt
} >$tmp
mv $tmp $out
grep -c '^> ' $out
