# As bash, words that cannot be used are not expanded: a for or select
# list after an invalid name, and an operator's word for a parameter
# with no value. Each expansion counts in n.
n=0
for 0x in $((n+=1)); do :; done; echo "for $? n=$n"
select 0x in $((n+=1)); do :; done; echo "select $? n=$n"
f() { trap 'echo DEBUG $BASH_COMMAND' DEBUG; select 0y in x; do :; done; trap - DEBUG; }
f
n=0; : ${u%$((n+=1))} ${u#$((n+=1))}; echo "unset %# n=$n"
n=0; e=; : ${e%%$((n+=1))} ${e##$((n+=1))}; echo "null %# n=$n"
n=0; : ${u/$((n+=1))/$((n+=1))} ${u^$((n+=1))} ${u,,$((n+=1))}; echo "unset /^, n=$n"
n=0; : ${e/$((n+=1))/$((n+=1))} ${e^$((n+=1))} ${e,$((n+=1))}; echo "null /^, n=$n"
n=0; : ${u:$((n+=1)):$((n+=1))} ${e:$((n+=1))}; echo "substring n=$n"
n=0; : ${u%${x?not reached}} ${u/$((1/0))}; echo "errors n=$n"
set --
n=0; : ${@%$((n+=1))} ${*/$((n+=1))} "${@^$((n+=1))}"; echo "no args n=$n"
n=0; : ${@:$((n+=1))}; echo "no args : n=$n"
set -- ""
n=0; : ${@%$((n+=1))} ${*#$((n+=1))}; echo "one empty %# n=$n"
n=0; : ${@/$((n+=1))} ${*^$((n+=1))}; echo "one empty /^ n=$n"
set -- "" ""; IFS=
n=0; : ${@%$((n+=1))} "${*%$((n+=1))}"; echo "two empty n=$n"
IFS=' '
a=()
n=0; : ${a[@]%$((n+=1))} ${a[*]/$((n+=1))} ${a[0]^$((n+=1))}; echo "empty array n=$n"
a=("")
n=0; : ${a[@]%$((n+=1))} ${a[@]/$((n+=1))}; echo "one empty element n=$n"
declare -A h
n=0; : ${h[@]%$((n+=1))} ${h[@]/$((n+=1))}; echo "empty assoc n=$n"
r=u
n=0; : ${!r%$((n+=1))} ${!r/$((n+=1))}; echo "indirect n=$n"
n=0; : ${u[$((n+=1))]%$((n+=1))}; echo "subscript n=$n"
printf '<%s>' "${a[@]%x}" "${@%x}" "${u/x}"; echo
(set -u; : ${u%$((n+=1))}); echo "nounset $? n=$n"
