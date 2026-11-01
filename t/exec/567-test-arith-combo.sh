t() { "$@"; echo "$? <- $*"; }
x=4
t test $(( x * 2 )) -eq 8
t [ $(( x = 5 )) -eq "$x" ]
echo "x=$x"
t [ "$(( x > 3 ))" = 1 ]
t test $(( 10 % 3 )) -ne 0
i=0
while [ $i -lt 3 ]; do i=$(( i + 1 )); done; echo "i=$i"
while test "$i" -gt 0; do : $(( i-- )); done; echo "i=$i"
n=0
until [ "$n" -ge 3 ]; do let n++; done; echo "n=$n"
[ "$x" -gt 2 ] && (( x > 2 )) && [[ x -gt 2 ]] && echo all-three
