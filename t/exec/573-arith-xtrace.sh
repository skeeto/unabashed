exec 2>&1
set -x
x=3
(( x += 2 ))
(( x > 100 ))
let 'y = x * 2' z=1
for (( i = 0; i < 2; i++ )); do :; done
echo $(( x + y ))
declare -i d=1+1
set +x
