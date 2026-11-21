# A subscript that arithmetic skips without evaluating may nest shell
# syntax as deep as the stack allows, as bash's: past 64 levels of $(
# it is no bad subscript, nor past 32 of $(( )) inside a subscript
for n in 64 65 1000; do
    o= c=
    for ((i = 0; i < n; i++)); do o+='$('; c+=')'; done
    x="0 && a[$o$c]"
    echo "$n: $((x)) $?"
    y="1 || a[\${b[$o$c]}]"
    echo "$n: $((y)) $?"
done
a=(7)
for n in 32 33 1000; do
    e=0
    for ((i = 0; i < n; i++)); do e="a[\$(($e))]"; done
    echo "$n: $((e)) $?"
done
