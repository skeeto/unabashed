== stdout 299
main same
comsub differs
subshell differs
dollar same
hello hello
infunc
sub
infunc
infunc []
infunc
A:echo "A:$BASH_COMMAND"
F:echo "A:$BASH_COMMAND"
X:x=$BASH_COMMAND
Z:echo "$BASH_COMMAND"
S:echo "S:$BASH_COMMAND"
R:echo "R:$BASH_COMMAND"
1
0 1000
0 1000
0 1 1000
0 1000
1
0 []
slept
seconds set

== stderr 63
infunc: line 13: ((: 1/0 : division by 0 (error token is "0 ")

== merged 362
main same
comsub differs
subshell differs
dollar same
hello hello
infunc
sub
infunc
infunc []
infunc
infunc: line 13: ((: 1/0 : division by 0 (error token is "0 ")
A:echo "A:$BASH_COMMAND"
F:echo "A:$BASH_COMMAND"
X:x=$BASH_COMMAND
Z:echo "$BASH_COMMAND"
S:echo "S:$BASH_COMMAND"
R:echo "R:$BASH_COMMAND"
1
0 1000
0 1000
0 1 1000
0 1000
1
0 []
slept
seconds set

== status 0
