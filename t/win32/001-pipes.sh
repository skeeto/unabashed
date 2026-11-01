# Pipes between the shell and programs: what a reader leaves is there
# for the next one, whoever reads first. $U is the shell under test,
# used as an external program.
"$U" -c 'printf "a\nb\nc\n"' | { read x; "$U" -c 'read y; echo "child $y"'; read z; echo "$x $z"; }
"$U" -c 'printf "1\n2\n3\n4\n"' | { read a; cat; echo "after $a"; }
f() { echo one; echo two; }
f | "$U" -c 'read l; echo "got $l"; read l; echo "got $l"'
echo out | { "$U" -c 'cat'; echo "st $?"; }
x=$("$U" -c 'echo captured; echo err >&2' 2>&1)
echo "$x"
"$U" -c 'cat' <<END
heredoc to a program
END
# A pipe that fills: the shell reads while the program writes
"$U" -c 'i=0; while [ $i -lt 3000 ]; do echo line$i; i=$((i+1)); done' | {
    n=0
    while read l; do n=$((n+1)); done
    echo "lines $n"
}
# Many lines read a line at a time from a program's pipe
"$U" -c 'for i in 1 2 3 4 5 6 7 8 9; do echo "$i$i$i"; done' | while read l; do printf '%s ' "$l"; done
echo
# Both ends busy at once: shell stages and programs mixed
seq 5 | "$U" -c 'while read n; do echo $((n*n)); done' | { sum=0; while read v; do sum=$((sum+v)); done; echo "sum $sum"; }
