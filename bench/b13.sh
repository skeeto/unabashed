# B13: printf in a loop to a file and into a variable, integers,
# strings and floats
f=${TMPDIR:-/tmp}/unabashed-b13
for ((i=1; i<=100000; i++)); do printf '%5d %s\n' $i foo; done >"$f"
wc -l <"$f"
for ((i=1; i<=100000; i++)); do printf -v x '%5d %s|%-8s|' $i foo bar; done
echo "$x"
for ((i=1; i<=30000; i++)); do printf -v x '%.3f %e %g' $i.25 $i $((i*7)).5; done
echo "$x"
printf -v x '%s,' {1..100000}; echo ${#x}
