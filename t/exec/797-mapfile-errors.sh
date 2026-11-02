# mapfile: option and target errors
exec 2>&1
mapfile -x; echo $?
readarray -q; echo $?
mapfile -u; echo $?
mapfile -u x a; echo $?
mapfile -u -1 a; echo $?
mapfile -u 9 a; echo $?
mapfile -n x a; echo $?
mapfile -n -1 a; echo $?
mapfile -O x a; echo $?
mapfile -O -1 a; echo $?
mapfile -s x a; echo $?
mapfile -s -2 a; echo $?
mapfile -c 0 a; echo $?
mapfile -c x a; echo $?
mapfile -C; echo $?
mapfile -n 99999999999 a < /dev/null; echo $?
mapfile '' < /dev/null; echo $?
mapfile 'a-b' < /dev/null; echo $?
mapfile 'a[1]' < /dev/null; echo $?
declare -A as; mapfile as <<< x; echo $?
readonly ro; mapfile ro <<< x; echo $?
mapfile -- a <<< y; echo $? "${a[0]}"
