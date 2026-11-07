# let takes an associative subscript to its first ] (compat 52 and
# later): a key whose quoting is left open cannot be expanded, and is
# reported twice and read as unset, its assignment not a valid
# identifier, while the expression goes on; with assoc_expand_once or
# under compat 51 it is other errors or a key as it is
exec 2>&1
declare -A a; b="80's"
let "++a[$b]"; echo "s=$? ${!a[@]}"
shopt -s assoc_expand_once 2>/dev/null; let "++a[$b]"; echo "s=$? ${!a[@]}"
shopt -u assoc_expand_once
BASH_COMPAT=51; let "++a[$b]"; echo "s=$? ${!a[@]}"
b='x"y'; let "++a[$b]"; echo "s=$? ${!a[@]}"
unset BASH_COMPAT; let "++a[$b]"; echo "s=$? ${!a[@]}"
(( a[$b]++ )); echo "s=$? ${!a[@]}"
echo $(( a[$b] ))
