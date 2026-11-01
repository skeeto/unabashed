# break and continue inside pipeline stages
for i in 1 2; do
    break | echo "pipe right $i"
    echo "loop $i"
done
for i in 1 2; do
    echo x | continue
    echo "loop2 $i"
done
for i in 1 2 3; do
    echo "$i" | while read n; do [ $n = 2 ] && break; echo "read $n"; done
done
for i in 1 2; do
    { break; echo "group in pipe $i"; } | { read l; echo "read [$l]"; }
done
for i in 1 2; do
    echo x | { continue; echo "group right $i"; }
done
