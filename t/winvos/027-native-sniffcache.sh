# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 10).
# The cache of what files hold (spawn.cpp's sniff) starts over when it
# fills, at any fill: many programs of many name lengths, each run once.
mkdir d
i=0
while [ $i -lt 1500 ]; do
    n=p$i
    [ $((i%3)) = 0 ] && n=$n-longer-name
    [ $((i%7)) = 0 ] && n=$n-and-a-much-longer-name-than-that
    cp C:/w64/bin/argv.exe d/$n.exe
    d/$n >/dev/null || echo "fail $n"
    i=$((i+1))
done
echo "ran $i"
