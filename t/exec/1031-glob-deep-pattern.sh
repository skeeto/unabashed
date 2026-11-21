# Each globbed directory component of a pattern recurses: hundreds
# match nothing, as in bash; far more crash bash, and the
# level globbing it dies the same way rather than the whole shell
mkdir -p d/d
p=$(printf '*/%.0s' $(seq 300))
x=($p); echo "${#x[@]} ${#x[0]}"
shopt -s nullglob
x=($p); echo "${#x[@]}"
x=(d/$p) ; echo "${#x[@]}"
shopt -u nullglob
p='*/'; for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17; do p=$p$p; done
{ (x=($p); echo not reached); } 2>/dev/null; echo "status $?"
{ echo $p | wc -c; } 2>/dev/null
echo end
