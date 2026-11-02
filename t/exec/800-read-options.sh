# read: -n and -N count characters in UTF-8, null bytes are dropped,
# and the options that need a terminal do nothing without one
LC_ALL=C.UTF-8
printf 'éàüx\n' | { read -n 2 a; echo "[$a]"; cat; }
printf 'éàüx\n' | { read -N 3 a; echo "[$a]"; cat; }
read -n 2 a <<< 'éàü'; echo "[$a]"
read -N 2 a <<< 'éàü'; echo "[$a]"
printf '\xff\xfeab\n' | { read -n 2 a; printf '%s' "$a" | od -An -tx1; cat; }
printf 'é' | { read -n 1 a; echo "[$a] $?"; }
printf '\xc3' | { read -n 1 a; printf '%s' "$a" | od -An -tx1; }
LC_ALL=C
printf 'éàüx\n' | { read -n 2 a; printf '%s' "$a" | od -An -tx1; cat; }
printf ' a b  c \n' | { read -N 7 x y; echo "[$x][$y]"; }
printf 'a\bc\n' | { read -N 3 x; echo "[$x]"; }
printf 'a\\nbc\n' | { read -N 3 x; echo "[$x]"; }
printf 'ab\ncd' | { read -N 4 x; echo "[$x]" $?; }
printf 'ab\ncd' | { read -n 4 x; echo "[$x]" $?; }
printf 'ab' | { read -N 4 x; echo "[$x]" $?; }
printf 'a b c\n' | { read -a arr -n 3; declare -p arr; }
printf '  a  b  \n' | { IFS= read -a arr; declare -p arr; }
printf 'a:b::c:\n' | { IFS=: read -a arr; declare -p arr; }
printf 'a:b::c::\n' | { IFS=: read x y z; echo "[$x][$y][$z]"; }
printf 'a:b::c:: \n' | { IFS=': ' read x y z; echo "[$x][$y][$z]"; }
printf 'one\0two\0' | { read -d '' x; read -d '' y; echo "[$x][$y]"; }
printf 'a\0b\n' | { read x; printf '%s' "$x" | od -An -c; }
printf 'x\\0y\n' | { read x; printf '%s' "$x" | od -An -c; }
read -n 0 x <<< abc; echo "[$x] $?"
read -N 0 x <<< abc; echo "[$x] $?"
read -t 0 x <<< abc; echo "$?"
read -p 'prompt> ' x <<< abc; echo "[$x]"
read -s -e -i init x <<< abc; echo "[$x]"
read -E x <<< abc; echo "[$x] $?"
read -t -1 x <<< abc; echo $?
read -t 1.5x x <<< abc; echo $?
read -n -1 x <<< abc; echo $?
read -n 1x x <<< abc; echo $?
read -u 1x x <<< abc; echo $?
read -d 2>&1; echo $?
read -a; echo $?
read 'a b' <<< x; echo $?
read a 'b c' <<< 'x y'; echo $? "[$a]"
read -a 'a[1]' <<< x; echo $?
read 'a[1]' <<< x; echo $? "${a[1]}"
read -r 'a[2]' 'a[3]' <<< 'p q'; declare -p a
unset a; read -a a <<< ''; declare -p a
