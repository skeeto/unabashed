# mapfile and readarray: lines into an indexed array
printf 'a\nb\n' | { mapfile; declare -p MAPFILE; }
printf 'a\nb\n' | { readarray -t x; declare -p x; }
mapfile -t a <<< $'one\ntwo\n\nfour'; declare -p a
mapfile -s 2 -n 2 a <<< $'1\n2\n3\n4\n5'; declare -p a
mapfile -n 0 a <<< $'1\n2'; declare -p a
a=(x y z w); mapfile -O 1 -t a <<< $'p\nq'; declare -p a
a=(x y z w); mapfile -t a <<< $'p\nq'; declare -p a
s=scalar; mapfile -O 1 s <<< x; declare -p s
s=scalar; mapfile s <<< x; declare -p s
printf 'a\0b\nc\0d\0' | { mapfile a; declare -p a; }
printf 'a\0b\nc\0d\0' | { mapfile -d '' a; declare -p a; }
printf 'a\0b\nc\0d\0' | { mapfile -td '' a; declare -p a; }
printf 'axbxx' | { mapfile -d xyz -t a; declare -p a; }
printf 'line\r\nlast' | { mapfile -t a; declare -p a; }
s=$'a\xffb\xffc\xff'; mapfile -t -d $'\xff' a <<< "$s"; declare -p a
mapfile a < /dev/null; declare -p a
mapfile -t -u 3 b 3<<< $'from 3\nx' <<< 'from 0'; declare -p b
declare -i ints; mapfile -t ints <<< $'1+2\n3*4'; declare -p ints
declare -u up; mapfile -t up <<< $'ab\ncd'; declare -p up
f() { local -a loc; mapfile -t loc <<< $'l1\nl2'; declare -p loc; }; f; declare -p loc 2>&1
mapfile -t a b <<< x; echo $? "${a[*]}" "${b-unset}"
seq 1 20000 > big; mapfile -t big < big; echo ${#big[@]} ${big[19999]}
printf '%s\n' {1..10} | { mapfile -t -n 3 a; mapfile -t -n 2 b; cat; declare -p a b; }
printf '%s\n' {1..10} > ten; { mapfile -t -n 3 a; read x; echo "$x"; cat; } < ten
