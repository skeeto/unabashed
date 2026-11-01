# ${name@A} and ${name[@]@A} for empty and declared-only arrays and for
# scalars
b=(); echo "${b[@]@A}"; echo "${b[*]@A}"; echo "${b@A}"
declare -A c=(); echo "${c[@]@A}"
declare -a e; echo "${e[@]@A}"; echo "${e@A}"
declare -ai e2; echo "${e2[@]@A}"
f=x; echo "${f[@]@A}"; echo "${f@A}"; echo "${f[0]@A}"
export g=1; echo "${g[@]@A}"
k=(1 2); echo "${k[1]@A}"; echo "${k@A}"
declare u; echo "[${u[@]@A}]" "[${u@A}]"
echo "[${nosuch[@]@A}]"
