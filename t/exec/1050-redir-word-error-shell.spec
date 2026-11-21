== stdout 13755
== set -u; echo hi >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; : >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; f(){ echo f; }; f >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; { echo g; } >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; while false; do :; done >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; exec >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; y=1 >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; builtin echo >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; command echo >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; command exec cat </dev/null >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; f(){ echo hi >${x?msg}; echo in $?; }; f; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; cat </dev/null >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; command cat </dev/null >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; nosuch >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; cat(){ :; }; command cat </dev/null >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; y=1 <${x?msg}; echo st $?
bash: line 1: x: msg
st 1
exit 0
bash: line 1: x: msg
st 1
posix exit 0
== set -u; {v}>${x?msg}; echo st $?
bash: line 1: x: msg
st 1
exit 0
bash: line 1: x: msg
st 1
posix exit 0
== set -u; ( echo in ) >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; f() ( echo in ) >${x?msg}; f; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; (echo hi >${x?msg}; echo in $?); echo st $?
bash: line 1: x: msg
st 1
exit 0
bash: line 1: x: msg
st 1
posix exit 0
== set -u; v=$(echo hi >${x?msg}; echo in $?); echo "[$v]"; echo st $?
bash: line 1: x: msg
[]
st 0
exit 0
bash: line 1: x: msg
[]
st 0
posix exit 0
== set -u; echo hi >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; : >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; f(){ echo f; }; f >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; { echo g; } >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; while false; do :; done >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; exec >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; y=1 >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; builtin echo >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; command echo >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; command exec cat </dev/null >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; f(){ echo hi >$u; echo in $?; }; f; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; cat </dev/null >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; command cat </dev/null >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; nosuch >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; cat(){ :; }; command cat </dev/null >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; y=1 <$u; echo st $?
bash: line 1: u: unbound variable
st 1
exit 0
bash: line 1: u: unbound variable
st 1
posix exit 0
== set -u; {v}>$u; echo st $?
bash: line 1: u: unbound variable
st 1
exit 0
bash: line 1: u: unbound variable
st 1
posix exit 0
== set -u; ( echo in ) >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; f() ( echo in ) >$u; f; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; (echo hi >$u; echo in $?); echo st $?
bash: line 1: u: unbound variable
st 1
exit 0
bash: line 1: u: unbound variable
st 1
posix exit 0
== set -u; v=$(echo hi >$u; echo in $?); echo "[$v]"; echo st $?
bash: line 1: u: unbound variable
[]
st 0
exit 0
bash: line 1: u: unbound variable
[]
st 0
posix exit 0
== set -u; echo hi >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; : >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; f(){ echo f; }; f >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; { echo g; } >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; while false; do :; done >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; exec >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; y=1 >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; builtin echo >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; command echo >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; command exec cat </dev/null >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; f(){ echo hi >$((1+)); echo in $?; }; f; echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; cat </dev/null >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; command cat </dev/null >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; nosuch >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; cat(){ :; }; command cat </dev/null >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; y=1 <$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
posix exit 0
== set -u; {v}>$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
posix exit 0
== set -u; ( echo in ) >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; f() ( echo in ) >$((1+)); f; echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; (echo hi >$((1+)); echo in $?); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
posix exit 0
== set -u; v=$(echo hi >$((1+)); echo in $?); echo "[$v]"; echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
[]
st 0
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
[]
st 0
posix exit 0
== set -u; echo hi >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; : >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; f(){ echo f; }; f >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; { echo g; } >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; while false; do :; done >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; exec >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; y=1 >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; builtin echo >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; command echo >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; command exec cat </dev/null >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; f(){ echo hi >${#x:1}; echo in $?; }; f; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; cat </dev/null >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; command cat </dev/null >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; nosuch >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; cat(){ :; }; command cat </dev/null >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; y=1 <${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 1
posix exit 0
== set -u; {v}>${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 1
posix exit 0
== set -u; ( echo in ) >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; f() ( echo in ) >${#x:1}; f; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; (echo hi >${#x:1}; echo in $?); echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 1
posix exit 0
== set -u; v=$(echo hi >${#x:1}; echo in $?); echo "[$v]"; echo st $?
bash: line 1: ${#x:1}: bad substitution
[]
st 0
exit 0
bash: line 1: ${#x:1}: bad substitution
[]
st 0
posix exit 0
script: line 1: ${#x:1}: bad substitution
next 1
script: line 3: ${#x:1}: bad substitution
st 1
exit 0

== stderr 0

== merged 13755
== set -u; echo hi >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; : >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; f(){ echo f; }; f >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; { echo g; } >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; while false; do :; done >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; exec >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; y=1 >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; builtin echo >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; command echo >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; command exec cat </dev/null >${x?msg}; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; f(){ echo hi >${x?msg}; echo in $?; }; f; echo st $?
bash: line 1: x: msg
exit 127
bash: line 1: x: msg
posix exit 127
== set -u; cat </dev/null >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; command cat </dev/null >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; nosuch >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; cat(){ :; }; command cat </dev/null >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; y=1 <${x?msg}; echo st $?
bash: line 1: x: msg
st 1
exit 0
bash: line 1: x: msg
st 1
posix exit 0
== set -u; {v}>${x?msg}; echo st $?
bash: line 1: x: msg
st 1
exit 0
bash: line 1: x: msg
st 1
posix exit 0
== set -u; ( echo in ) >${x?msg}; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; f() ( echo in ) >${x?msg}; f; echo st $?
bash: line 1: x: msg
st 127
exit 0
bash: line 1: x: msg
st 127
posix exit 0
== set -u; (echo hi >${x?msg}; echo in $?); echo st $?
bash: line 1: x: msg
st 1
exit 0
bash: line 1: x: msg
st 1
posix exit 0
== set -u; v=$(echo hi >${x?msg}; echo in $?); echo "[$v]"; echo st $?
bash: line 1: x: msg
[]
st 0
exit 0
bash: line 1: x: msg
[]
st 0
posix exit 0
== set -u; echo hi >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; : >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; f(){ echo f; }; f >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; { echo g; } >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; while false; do :; done >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; exec >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; y=1 >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; builtin echo >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; command echo >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; command exec cat </dev/null >$u; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; f(){ echo hi >$u; echo in $?; }; f; echo st $?
bash: line 1: u: unbound variable
exit 127
bash: line 1: u: unbound variable
posix exit 127
== set -u; cat </dev/null >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; command cat </dev/null >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; nosuch >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; cat(){ :; }; command cat </dev/null >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; y=1 <$u; echo st $?
bash: line 1: u: unbound variable
st 1
exit 0
bash: line 1: u: unbound variable
st 1
posix exit 0
== set -u; {v}>$u; echo st $?
bash: line 1: u: unbound variable
st 1
exit 0
bash: line 1: u: unbound variable
st 1
posix exit 0
== set -u; ( echo in ) >$u; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; f() ( echo in ) >$u; f; echo st $?
bash: line 1: u: unbound variable
st 127
exit 0
bash: line 1: u: unbound variable
st 127
posix exit 0
== set -u; (echo hi >$u; echo in $?); echo st $?
bash: line 1: u: unbound variable
st 1
exit 0
bash: line 1: u: unbound variable
st 1
posix exit 0
== set -u; v=$(echo hi >$u; echo in $?); echo "[$v]"; echo st $?
bash: line 1: u: unbound variable
[]
st 0
exit 0
bash: line 1: u: unbound variable
[]
st 0
posix exit 0
== set -u; echo hi >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; : >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; f(){ echo f; }; f >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; { echo g; } >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; while false; do :; done >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; exec >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; y=1 >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; builtin echo >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; command echo >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; command exec cat </dev/null >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; f(){ echo hi >$((1+)); echo in $?; }; f; echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
exit 1
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
posix exit 127
== set -u; cat </dev/null >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; command cat </dev/null >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; nosuch >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; cat(){ :; }; command cat </dev/null >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; y=1 <$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
posix exit 0
== set -u; {v}>$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
posix exit 0
== set -u; ( echo in ) >$((1+)); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; f() ( echo in ) >$((1+)); f; echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 127
posix exit 0
== set -u; (echo hi >$((1+)); echo in $?); echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
st 1
posix exit 0
== set -u; v=$(echo hi >$((1+)); echo in $?); echo "[$v]"; echo st $?
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
[]
st 0
exit 0
bash: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
[]
st 0
posix exit 0
== set -u; echo hi >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; : >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; f(){ echo f; }; f >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; { echo g; } >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; while false; do :; done >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; exec >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; y=1 >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; builtin echo >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; command echo >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; command exec cat </dev/null >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; f(){ echo hi >${#x:1}; echo in $?; }; f; echo st $?
bash: line 1: ${#x:1}: bad substitution
exit 1
bash: line 1: ${#x:1}: bad substitution
posix exit 127
== set -u; cat </dev/null >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; command cat </dev/null >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; nosuch >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; cat(){ :; }; command cat </dev/null >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; y=1 <${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 1
posix exit 0
== set -u; {v}>${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 1
posix exit 0
== set -u; ( echo in ) >${#x:1}; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; f() ( echo in ) >${#x:1}; f; echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 127
posix exit 0
== set -u; (echo hi >${#x:1}; echo in $?); echo st $?
bash: line 1: ${#x:1}: bad substitution
st 1
exit 0
bash: line 1: ${#x:1}: bad substitution
st 1
posix exit 0
== set -u; v=$(echo hi >${#x:1}; echo in $?); echo "[$v]"; echo st $?
bash: line 1: ${#x:1}: bad substitution
[]
st 0
exit 0
bash: line 1: ${#x:1}: bad substitution
[]
st 0
posix exit 0
script: line 1: ${#x:1}: bad substitution
next 1
script: line 3: ${#x:1}: bad substitution
st 1
exit 0

== status 0
