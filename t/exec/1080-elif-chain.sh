# elif arms are parsed, copied and run as a flat sequence: a generated
# chain as long as bash's parser takes picks the right arm, at the top,
# in a function, a pipeline stage and a background job, and prints as
# the nested else/if bash makes of it
n=2000
s=$(for i in $(seq $n); do printf 'elif [ "$x" = %d ]; then echo "hit %d"; ' $i $i; done)
eval "pick() { if [ \"\$x\" = 0 ]; then echo zero; $s else echo none; fi; }"
for x in 0 1 1000 $n 99999; do pick; done
x=$n; pick | cat
x=7; pick & wait
eval "if false; then :; $s fi"; echo "status $?"
x=3; eval "if false; then :; $s fi"
# the same chain inside a command substitution, and printed
y=$(x=1999; eval "if false; then :; $s fi"); echo "$y"
f() {
    if [ "$1" = a ]; then echo A
    elif [ "$1" = b ]; then echo B
    elif [ "$1" = c ]; then echo C; else echo other
    fi
}
declare -f f
f c; f d
# long case lists and [[ ]] chains
s=$(printf 'k%d) echo "k%d";; ' $(seq $n | sed 'p'))
eval "c() { case \$1 in $s *) echo default;; esac; }"
c k1; c k$n; c z
s=$(printf -- '-n x && %.0s' $(seq $n))
eval "[[ $s -n x ]]"; echo "cond $?"
eval "[[ $s -z x ]]"; echo "cond $?"
