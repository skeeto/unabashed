# read: the rest of the line loses trailing IFS whitespace, escaped or not
echo " a  b\ " | ( read x ; echo -"$x"- )
echo " a  b c\  " | ( read x y; echo -"$x"-"$y"- )
echo "a\ " | ( read x ; echo -"$x"- )
# IFS whitespace is any space character in IFS, here and in splitting
printf '  line\tb \t\r\f\v\n' | { IFS=$'\t\r\f\v' read v1 v2; printf '[%s][%s]\n' "$v1" "$v2"; }
printf 'a\v\fb\v\n' | { IFS=$'\v\f' read a b; printf '[%s][%s]\n' "$a" "$b"; }
IFS=$'\v'; x=$'\va\v\vb\v'; set -- $x; echo $#
IFS=$'\f:'; x=$'\fa\f:\fb\f'; set -- $x; echo $# "$1" "$2"
IFS=$' \t\n'
# assigning IFS does not change how the line splits
IFS=:; read -r IFS var rest <<<',:abc:def,ghi'
printf 'IFS=[%s] var=[%s] rest=[%s]\n' "$IFS" "$var" "$rest"
IFS=:; read -r IFS var rest <<<' :  abc:def,ghi '
printf 'IFS=[%s] var=[%s] rest=[%s]\n' "$IFS" "$var" "$rest"
IFS=$' \t\n'
# failed assignments: 2 except for the last variable
readonly r; read r <<< a; echo $?
read a r <<< "a b"; echo $?
read r a <<< "a b"; echo $?
read -a r <<< a; echo $?
declare -A as; read -a as <<< a; echo $?
(readonly REPLY; read <<< a; echo $?)
