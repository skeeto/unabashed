# A nameref to a[@] or a[*] is a list as $n, like "${a[@]}", but in
# braces bash finds no variable of that name and expands the reference
# for one word: the elements joined (a[@] by IFS's first character or a
# space, a space where nothing splits; a[*] quoted as "$*"), unset when
# there are none. The operators work on that string; a length counts
# the elements; @a and @A find no variable, as for a nameref to an
# element.
a=(one 'two  x' three)
declare -n n='a[@]' s='a[*]'
p() { printf '<%s>' "$@"; echo; }
p "$n" $n "${n}" ${n} "${n[@]}"
p "${#n}" "${n:1}" "${n:1:5}" "${n: -3}" "${n/o/X}" "${n//o/X}" "${n#o}" "${n%e}"
p "${n^^}" "${n@Q}" "${n@U}" "${n@K}" "${n-unset}" "${n:+set}" "${!n}"
p "${n@a}" "${n@A}" "${s@a}"
for i in "$n"; do p "$i"; done
for i in "${n}"; do p "$i"; done
IFS=,
p "${n}" ${n} "${s}" ${s} "$s" "${n:0}" "${n#*,}" "${s/,/-}"
x=${n}; y=${s}; z="${n}"; p "$x" "$y" "$z"
case ${n} in "one two  x three") echo case-joined;; *) echo case-other;; esac
[[ ${n} == "one two  x three" ]] && echo cond-joined
IFS=
p "${n}" ${n} "${s}" ${s}
unset IFS
p "${n}" ${n} "${s}" ${s}
declare v=${n}; x=( ${n} ); y=( "${n}" ); p "$v" "${x[@]}" "${y[@]}"
cat <<EOF
here ${n} $n
EOF
declare -A h=([k]=v)
declare -n hn='h[@]'
p "${hn}" "$hn" "${#hn}" "${hn:1}" "${hn/v/V}"
e=()
declare -n en='e[@]' es='e[*]' un='nosuch[@]'
p "${en}" ${en} "${es}" "$en" "${#en}" "${en-unset}" "${en:-empty}" "${en+set}"
p "${un}" ${un} "$un" "${#un}" "${un-unset}" "${un+set}"
declare -n r='a[1]'
p "${r}" "${r@a}" "${r@A}" "${r@Q}"
( p "${en:=x}" ) 2>&1
( p "${en:?msg}" ) 2>&1
set -u
p "$en" "$un"
{ echo "${#un}"; echo notreached; }
echo "status $?"
( p "${en}" ) 2>&1
( p "${un#x}" ) 2>&1
p "${n}"
