# Longer pipelines: external stages started directly, shell stages in
# between, and what each stage sees
seq 10 | grep 1 | sort -r | head -2
true | echo "mid $BASH_SUBSHELL" | cat
true | { echo "group $BASH_SUBSHELL"; } | cat
true | (echo "paren $BASH_SUBSHELL") | cat
echo x | nosuchcmd | cat; echo "st=$?"
echo x | cat | nosuchcmd; echo "st=$?"
printf '%s\n' b a c | LC_ALL=C sort | tr a-z A-Z
v=val; echo "$v" | tr a-z A-Z | cat
x=1; echo z | x=2 cat | cat; echo "x=$x"
n=$(seq 5 | wc -l | tr -d ' '); echo "n=$n"
seq 3 | cat | cat | cat | cat | tail -1
f() { tr a-z A-Z; }
echo hello | f | cat
echo hello | cat | f
seq 3 | while read l; do echo "$l"; done | sort -r | while read m; do echo "<$m>"; done
for w in one two; do echo "$w"; done | cat -n | sed 's/^ *//'
seq 4 | { read a; echo "a=$a"; cat | tr '\n' ' '; echo; }
! echo abc | grep -q z; echo "inv=$?"
