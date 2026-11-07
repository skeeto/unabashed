# P3 (POSIX): reading a file a line at a time, and splitting fields
f=${TMPDIR:-/tmp}/unabashed-p3.$$
i=0; while [ $i -lt 50000 ]; do echo "$i alpha beta:gamma $((i%13))"; i=$((i+1)); done >"$f"
n=0; while read -r a b c d; do n=$((n+d)); done <"$f"; echo $n
n=0; while IFS=: read -r a b; do n=$((n+${#b})); done <"$f"; echo $n
rm -f "$f"
