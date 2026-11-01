# counting lines including blank and whitespace-only ones
printf 'a\n\n  \n\tb\nlast' >/tmp/f
n=0
while IFS= read -r l || [ -n "$l" ]; do
    n=$((n+1))
    echo "$n:[$l]:${#l}"
done </tmp/f
m=0
while read -r l; do m=$((m+1)); done </tmp/f
echo "m=$m l=[$l]"
