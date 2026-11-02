# test -N follows reads and writes (relatime), stdin and stdout
exec 2>&1
cd /tmp
touch t825; sleep 1; echo hello > t825
test -N t825; echo "N after write $?"
cat t825 >/dev/null
test -N t825; echo "N after read $?"
sleep 1; echo more >> t825
test -N t825; echo "N after append $?"
test -G t825; echo "G $?"
test -r /dev/stdin; echo "r stdin $?"
test -w /dev/stdout; echo "w stdout $?"
rm -f t825
