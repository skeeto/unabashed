# Arithmetic at bash's recursion limit whose subscripts expand a
# parameter and an arithmetic expansion at every level takes far more
# stack than plain recursion; it still ends in the limit's error, in a
# subshell, a pipeline stage and a nested shell alike
b='a[${c[$((b))]}]'
(echo $((b)); echo not reached); echo "status $?"
(echo $((b)); echo not reached) | cat; echo "pipe ${PIPESTATUS[*]}"
bash -c 'b='\''a[${c[$((b))]}]'\''; echo $((b)); echo not reached'; echo "nested $?"
# up to the limit, then nesting to its limit at the bottom
w=$(printf '%.0s(' {1..1000})1$(printf '%.0s)' {1..1000})
n=300 d='n-- > 0 ? a[${c[$((d))]}] : w'
(echo "deep $((d))")
echo done
