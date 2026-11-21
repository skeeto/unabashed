# A pipeline of thousands of stages, as long as bash's parse takes,
# prints without the printer recursing for each stage: by declare -f,
# also from deep in a recursion, and as a command substitution's body,
# which the parse prints
s=
for ((i = 0; i < 3300; i++)); do s+='echo | '; done
eval "f() { $s cat; }"
declare -f f | wc -c
deep() { if (($1 > 0)); then deep $(($1-1)); else declare -f f | wc -c; fi; }
deep 2000
eval "h() { x=\$($s cat); }"
declare -f h | wc -c
eval "g() { : <<E | $s cat <<F
one
E
two
F
}"
declare -f g | head -3
declare -f g | tail -6 | cut -c1-24
