# loops with both input and output redirected
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
printf '%s\n' 1 2 3 >/tmp/in
while read n; do echo "sq $((n*n))"; done </tmp/in >/tmp/out
show /tmp/out
for w in a b; do echo "$w"; done >>/tmp/out
show /tmp/out
while read n; do
    echo "line $n"
    echo "err $n" >&2
done </tmp/in 2>/tmp/err >/tmp/out2
show /tmp/out2
show /tmp/err
for i in 1 2; do
    while read n; do echo "$i$n"; done </tmp/in
done
