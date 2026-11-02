# ${!N} of unset positionals and specials; ${!name[@]} with operators;
# @a and @A of variables with attributes but no value
exec 2>&1
z=Z
echo "1 ${!9:-$z} ${!9-$z} ${!9:+x}|"
unset x; echo "2 ${!x:-$z}"
x=; echo "3 ${!x:-$z}"
a=(1); echo "4 ${!a[3]:-dflt}"
echo "5 ${!nosuch[3]:-dflt}"
echo "6 ${!9#a}|"
VAR4=(aaa bbb); varname=VAR4
echo "7 ${!varname[@]} ${!varname[@]@Q} ${!VAR4[@]}"
echo "8 ${!VAR4[@]@Q}"
VAR5=(aaa bbb); varname='VAR5[@]'
echo "9 ${!varname[@]%b} ${!varname##aa}"
declare v; echo "10 ${v@A}|${v[@]@A}|${v@a}|${v[@]@a}|${v[0]@a}"
declare -i w; echo "11 ${w@A}|${w[@]@A}|${w@a}|${w[@]@a}|${w[0]@A}"
declare -a e; echo "12 ${e@A}|${e[@]@A}|${e@a}|${e[@]@a}|${e[*]@A}"
declare -A h; echo "13 ${h@A}|${h[@]@A}|${h@a}|${h[@]@a}|${h[x]@a}"
declare -ar ra=(1 2); echo "14 ${ra[@]@a}|${ra[5]@a}|${ra[5]@A}"
s=1; echo "15 ${s[@]@a}|${s[3]@a}|${s[3]@A}|${s[@]@A}"
declare -lr VAR1; echo "16 ${VAR1@A}|${VAR1[@]@a}"
(set -u; declare -a n=([1]=x); echo "17 ${n@a} ${n@A}")
(set -u; declare -ia m=(); echo "18 ${m@a}")
(set -u; declare -i k; echo "19 ${k@a}")
(set -u; declare -a n=([1]=x); echo "20 ${n@Q}")
a=/a; echo "21 /${a///a/} ${a//\//-} ${a/#/x} ${a//a}"
b=//a; echo "22 ${b///} ${b////x}"
