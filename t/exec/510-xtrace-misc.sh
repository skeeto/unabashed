set -x
x=$(echo a) y=b
echo "${x}${y}"
: $((1 + 2))
printf '%s\n' "fmt"
declare -x d=1
local_outside=1
unset x
[[ -n $y ]] && echo y
true | echo piped
echo a\ b "c\"d"
set -- 'p 1' p2
echo "$@"
set +x
