# >(list): a pipe to a concurrent subshell. Its output is ordered here
# through a capture (which waits for every writer), a file read after
# wait, or a pipeline's reader.
x=$(echo hi > >(tr a-z A-Z)); echo "x=$x"
y=$(printf '%s\n' b a c | tee >(sort >&2) 2>&1 >/dev/null); echo "y=$y"
echo data > >(cat > out1); wait $!; cat out1
printf '%s\n' 3 1 2 > >(sort > out2); wait $!; cat out2
exec 4> >(cat > out3); echo via4 >&4; echo more >&4; exec 4>&-; wait $!; cat out3
z=$(echo start; echo inner > >(cat); wait $!; echo end); echo "$z"
{ echo to-procsub > >(cat); } | cat
f() { echo "$1" | sed 's/[0-9]*$/N/'; echo fn > "$1"; }; f >(cat > out4); wait $!; cat out4
echo final > >(read -r v; echo "read $v" > out5); wait $!; cat out5
w=$(echo one >(true) | wc -w); echo "w=$w"
