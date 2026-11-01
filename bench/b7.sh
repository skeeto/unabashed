for i in {1..10000}; do x=$(echo $i); done; echo $x
for i in {1..10000}; do x=$(printf '%d' $i); done; echo $x
for i in {1..10000}; do x=${ echo $i; }; done; echo $x
echo hello > /tmp/b7small.$$
for i in {1..10000}; do x=$(< /tmp/b7small.$$); done; echo $x
rm -f /tmp/b7small.$$
