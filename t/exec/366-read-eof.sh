# end of input, partial lines, empty lines
: >/tmp/empty
read x </tmp/empty; echo "status $? x=[$x]"
x=old; read x </tmp/empty; echo "x=[$x]"
printf 'partial' >/tmp/p
read x </tmp/p; echo "status $? x=[$x]"
printf '\n\nthird\n\n' >/tmp/e
n=0; while read x; do n=$((n+1)); echo "$n:[$x]"; done </tmp/e
printf '   \n' >/tmp/w
read x </tmp/w; echo "status $? x=[$x]"
read a b </tmp/p; echo "status $? a=[$a] b=[$b]"
read </tmp/p; echo "status $? REPLY=[$REPLY]"
