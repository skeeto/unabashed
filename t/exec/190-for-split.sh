# for loops iterate over the expanded and split words.
x='a b  c'
for w in $x; do printf '[%s]' "$w"; done; echo
for w in "$x"; do printf '[%s]' "$w"; done; echo
for w in $x"" '' $empty; do printf '[%s]' "$w"; done; echo
for w in $empty; do printf '[%s]' "$w"; done; echo none
set -- 1 '2 3' ''
for w; do printf '[%s]' "$w"; done; echo
for w in "$@"; do printf '[%s]' "$w"; done; echo
for w in $@; do printf '[%s]' "$w"; done; echo
for w in {a,b}{1,2}; do printf '[%s]' "$w"; done; echo
IFS=,
for w in $x,d,,e; do printf '[%s]' "$w"; done; echo
for w in a,b; do printf '[%s]' "$w"; done; echo
