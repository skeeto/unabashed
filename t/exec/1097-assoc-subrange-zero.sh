# Substring expansion of an associative array counts its elements from
# 1 (0 is the first too), and as bash's assoc_subrange a length of 0
# still takes the element at the offset; an indexed array and the
# positional parameters take none.
declare -A h=([a]=1 [b]=2 [c]=3 [d]=4)
a=(1 2 3 4)
set -- 1 2 3 4
p() { printf '<%s>' "$@"; echo; }
p "${!h[@]}"
for off in 0 1 2 4 5 ' -1' ' -4' ' -5'; do
    eval 'p "off'"$off"'" "${h[@]:'"$off"':0}" / "${h[*]:'"$off"':0}" / ${h[@]:'"$off"':0} / "${a[@]:'"$off"':0}" / "${@:'"$off"':0}"'
    eval 'p "off'"$off"'" "${h[@]:'"$off"':1}" / "${h[@]:'"$off"':2}" / "${h[@]:'"$off"'}"'
done
declare -A g=()
p "${g[@]:0:0}" "${g[@]:1:0}"
declare -A s=([k]='')
p "${s[@]:0:0}" "${s[@]:1:0}" "${s[@]:2:0}"
