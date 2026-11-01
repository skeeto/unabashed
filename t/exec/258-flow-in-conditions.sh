# break and continue from inside conditions and other compounds
for i in 1 2 3; do
    if [ $i = 2 ]; then continue; fi
    echo "if $i"
done
for i in 1 2 3; do
    case $i in 2) break;; esac
    echo "case $i"
done
for i in 1 2 3; do
    { [ $i = 2 ] && continue; echo "group $i"; }
done
i=0
while i=$((i+1)); [ $i = 2 ] && continue; [ $i -lt 4 ]; do
    echo "cond $i"
done
i=0
until i=$((i+1)); [ $i -ge 3 ] && break; false; do
    echo "until cond $i"
done
echo "i=$i"
for i in 1 2; do
    while break 2; do echo never; done
    echo never
done
echo "end $i"
for i in 1 2 3; do
    if [ $i = 2 ] && continue; then echo never; fi
    echo "if-cond $i"
done
