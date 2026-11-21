# Reading a script file or standard input, bash starts each top-level
# command with no redirected input for & to keep, so after
# "{ :; } </dev/null" on one line, a cat & on a later one gets
# /dev/null; within one line, and under -c or in a sourced file, it
# stays (the job reads the shell's input)
printf '{ :; } </dev/null\n' >s4.sh
n=0
run() {
    n=$((n+1))
    printf '%s\n' "$1" >c4.sh
    a=$(echo in | bash ./c4.sh 2>&1)
    b=$(echo in | bash -c "$1" 2>&1)
    c=$( { cat c4.sh; echo 'echo END'; } | bash 2>&1)
    d=$(echo in | bash -c '. ./c4.sh' 2>&1)
    echo $n file[$a] -c[$b] stdin[$c] source[$d]
}
run '{ :; } </dev/null
cat & wait'
run '{ :; } </dev/null; cat & wait'
run 'f() { { :; } </dev/null; }
f
cat & wait'
run 'eval "{ :; } </dev/null"
cat & wait'
run 'source ./s4.sh
cat & wait'
run 'eval "{ :; } </dev/null
cat & wait"'
run 'f() { { :; } </dev/null
cat & wait; }
f'
run '{ :; } </dev/null
{ cat & wait; }'
run 'if :; then { :; } </dev/null
fi
cat & wait'
run 'while { :; } </dev/null; do break; done
cat & wait'
run '{ :; } </dev/null
echo ${ cat & wait; }'
run 'for i in 1; do :; done </dev/null
cat & wait'
