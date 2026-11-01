# expansion order of assignments, words and redirections
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
n=0
echo $((n+=1)) >/tmp/f$((n+=10))
echo "n=$n"
show /tmp/f11
x=$(echo assign-err >&2) : 2>/dev/null
echo "x=[$x]"
: $(echo word-err >&2) 2>/dev/null
echo done
