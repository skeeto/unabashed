# A program writing more than a pipe holds to each of two or three
# command substitutions, in turns, is read on all of them at once
out=$( { err=$(bash -c 'for i in 1 2 3; do seq 20000; seq 30000 >&2; done' 2>&1 1>&3); echo "err ${#err} ${err: -5}"; } 3>&1 )
echo "1: ${#out} ${out: -16}"
x=${ exec 3>&1; y=${ seq 1 100000; }; echo "inner-bytes=${#y}"; }
echo "2: $x"
x=${ exec 3>&1; y=${ seq 1 100000 >&3; echo in-y; }; echo "y=[$y]"; }
echo "3: ${#x} ${x: -12}"
a=$( { b=$( { c=$(bash -c 'for i in 1 2; do seq 30000; seq 20000 >&3; seq 25000 >&4; done'); echo "c ${#c}"; } 3>&1 ); echo "b ${#b}"; } 4>&1 )
echo "4: ${#a} ${a: -16}"
