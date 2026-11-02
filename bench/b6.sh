# B6: read loops over a seekable file, a pipe and a process
# substitution, and mapfile -t, on 200,000 lines
f=${TMPDIR:-/tmp}/unabashed-b6
if [ ! -f "$f" ]; then
    for ((i=0; i<200000; i++)); do
        echo "line $i of the benchmark input, padded out a little"
    done >"$f.new" && mv "$f.new" "$f"
fi
n=0; while IFS= read -r l; do n=$((n+${#l})); done <"$f"; echo $n
cat "$f" | { n=0; while IFS= read -r l; do n=$((n+${#l})); done; echo $n; }
n=0; while read l; do n=$((n+1)); done < <(cat "$f"); echo $n
for r in 1 2 3 4 5; do mapfile -t a <"$f"; done; echo ${#a[@]} "${a[199999]}"
cat "$f" | { mapfile -t a; echo ${#a[@]}; }
