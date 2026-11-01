# Expansion details found by the bash test suite oracle: a long brace
# sequence in little memory, an indexed subscript evaluated once in
# a[expr]++ and a[expr]+=n, declare -i leaving the old value alone, and
# an unterminated $( in a quoted ${...} word, reported as it expands
x=({1..20000}); echo ${#x[@]} ${x[19999]}
echo {1..6000} | wc -c
i=0; (( a[i++]++ )); (( a[i++] += 5 )); (( ++a[i++] )); echo $i; declare -p a
j=3; (( b[j--]-- )); echo $j; declare -p b
v=3+4; declare -i v; echo $v; v+=1; echo $v
w=2*3; typeset -i w; declare -p w
a=4
echo "${a+'$('\'}"
echo next
echo "${a+'$(echo x'}"
echo after
