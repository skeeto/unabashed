# let: an associative subscript, already expanded, ends at the first ]
exec 2>&1
typeset -A a
b="80's"
shopt -s assoc_expand_once
let "++a[$b]"; echo "${a[$b]}"
let "a[$b]+=2"; echo "${a[$b]}"
declare -p a
c='x]y'
let "a[$c]=3"; echo $?
declare -p a
typeset -a i
let "i[1+1]=4"; declare -p i
