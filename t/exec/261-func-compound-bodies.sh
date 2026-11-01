# function bodies that are compound commands other than groups
f1() for w; do echo "for body $w"; done
f2() if [ "$1" = y ]; then echo "if yes"; else echo "if no"; fi
f3() while [ $# -gt 0 ]; do echo "while $1"; shift; done
f4() case $1 in a) echo "case a";; *) echo "case other";; esac
f5() ( echo "subshell body"; exit 3 )
f6() (( $1 > 2 ))
f7() [[ $1 == x* ]]
f8() until [ $# -eq 0 ]; do echo "until $1"; shift; done
f9() for ((i=0; i<$1; i++)); do echo "arith $i"; done
f1 a b; f2 y; f2 n
f3 p q; f4 a; f4 z
f5; echo "f5: $?"
f6 3; echo "f6 3: $?"; f6 1; echo "f6 1: $?"
f7 xyz; echo "f7: $?"; f7 abc; echo "f7 abc: $?"
f8 m n; f9 2
