# break and continue inside case items, including fallthrough
for w in a b c d; do
    case $w in
        a) echo "a"; continue;;
        b) echo "b falls";&
        c) echo "c body ($w)"; [ $w = c ] && break;;
        *) echo never;;
    esac
    echo "after case $w"
done
echo "w=$w"
for w in x y; do
    case $w in
        x) echo "x testnext";;&
        *) echo "star $w"; continue;&
        y) echo never;;
    esac
    echo never
done
i=0
while :; do
    i=$((i+1))
    case $i in
        1|2) continue;;
        5) break;;
    esac
    echo "i=$i"
done
