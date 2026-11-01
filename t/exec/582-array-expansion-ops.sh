a=(1 'x y' "" z); declare -A h=([k]=v [a b]="c d")
recho() { for x; do printf '<%s>' "$x"; done; echo; }
recho "${a[@]@A}"; recho ${a[@]@A}; recho "${a@A}"; recho "${a[1]@A}"
recho "${h[@]@A}"; recho "${h[@]@K}"; recho "${h[@]@k}"; recho "${a[@]@K}"; recho "${a[@]@k}"; recho "${a[*]@K}"
recho "${a[@]@Q}"; recho "${a[@]@a}" "${h@a}" "${h[k]@a}"
recho "${a[@]:1:2}" "${a[@]:-2}" "${a[*]:1}" ; recho "${!a[@]}" "${!h[*]}" "${!h[@]}"
b=([2]=a [5]=b [9]=c); recho "${b[@]:3}" "${b[@]:5:1}" "${b[@]: -2}" "${b[@]:0:2}"
recho "${a[@]#?}" "${a[*]/x/Q}" "${a[@]^^}" "${#a[@]}" "${#a[1]}" "${#h[@]}" "${#a}"
recho "${a[1]:1}" "${a:0:1}" ${a[@]+set} "${z[@]-unset}" "${a[@]:+alt}"
recho ${!a*} ${!h@}; x=a[1]; recho "${!x}"; recho "${a[-1]}" "${a[3]-d}" "${a[7]-d}"
recho "${h}" "${h[0]-nope}"; h[0]=zero; recho "$h" $a "${a}"
recho "${a[@]@E}"; set -u; recho "${a[7]}"; echo after
