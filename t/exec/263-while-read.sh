# read loops over a file, including a last line without a newline
printf 'one 1\ntwo 2\nthree' > /tmp/data
while read name num; do echo "[$name][$num]"; done < /tmp/data
echo "status $?"
while read -r line || [ -n "$line" ]; do echo "got: $line"; done < /tmp/data
while IFS= read -r line; do
    case $line in t*) echo "t-line: $line";; *) continue;; esac
done < /tmp/data
n=0
while read; do n=$((n+1)); [ $n = 2 ] && break; done < /tmp/data
echo "n=$n REPLY=$REPLY"
