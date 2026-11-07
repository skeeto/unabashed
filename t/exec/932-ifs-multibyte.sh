# IFS in a UTF-8 locale is taken a character at a time
export LC_ALL=en_US.UTF-8
euro=$'\342\202\254'
IFS=$'\254'; t=+$euro+; set -- $t; echo "$#"
IFS=$euro; t="a${euro}b${euro}${euro}c"; set -- $t; echo "$# $1 $2 $3 $4"
echo "$*"
IFS=$'\342'; t=$'x\342y'"$euro"; set -- $t; echo "$# $1"
IFS=":$euro"; read -r a b c <<<"1${euro}2:3"; echo "$a|$b|$c"
IFS=$euro read -ra arr <<<"p${euro}q"; echo "${#arr[@]} ${arr[1]}"
IFS=$'\254' read -r a b <<<"+$euro+"; echo "$a|$b"
LC_ALL=C; IFS=$'\254'; t=+$euro+; set -- $t; echo "C $#"
