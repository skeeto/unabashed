b=([9223372036854775807]=p q r); echo notreached
declare -p b
b=([9223372036854775806]=p q); declare -p b
b+=(r); echo notreached
declare -p b
f=(1 2); f[9223372036854775807]=z; echo "${f[-1]}" "${f[-2]}" "${f[@]: -1}" "${f[@]: -2:1}"
f[-1]+=y; declare -p f; f[-2]=w; declare -p f
unset 'f[-1]'; declare -p f
g=([1]=a [9223372036854775807]=b); g+=([-1]=c); declare -p g
mapfile -O 4294967296 m <<< x; echo st=$?
mapfile -O 4294967295 m <<< $'1\n2'; echo st=$?; declare -p m
f=(1 2); f[9223372036854775807]=z; printf "<%s>" "${f[-1]}" "${f[-2]}" "${f[@]: -1}" "${f[@]: -2:1}" "${f[@]: -3:2}"; echo
