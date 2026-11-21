# An alias whose expansion a syntax error, return or exit abandoned
# expands again afterwards, also once redefined, and in the shell around
# a subshell or command substitution where that happened
shopt -s expand_aliases
alias a='echo hello ( x y z'
eval 'a'
alias a='echo redefined'
a
alias b='echo hello ( x y z'
x=$(eval 'b' 2>&1); echo "x=[${x#*: }]"
alias b='echo bee'
eval 'b'
printf '%s\n' r 'echo after' > src.sh
alias r=$'return 3\n'
. ./src.sh; echo "st=$?"
alias r='echo r again'
r
f() { eval 'r'; echo "in f $?"; }
alias r=$'return 4\n'
f; echo "f $?"
alias r='echo r third'
r
alias x=$'exit 5\n'
(eval x); echo "sub $?"
alias x='echo x again'
x
alias y=$'exit 6\n'
z=$(eval y); echo "comsub $?"
alias y='echo y again'
y
