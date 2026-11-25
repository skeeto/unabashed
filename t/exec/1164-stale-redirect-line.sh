# A simple command that starts with a redirection takes its line from
# bash's simplecmd_lineno, which reset_parser sets: after a syntax error
# in eval, in a script without #! (the line it was run from), and in
# the EXIT trap (the line it runs at), until a simple command is parsed
eval ')'

<nope
eval ')'
:
<nope
eval ':; ('

{ <nope; }
printf '<nope\n<nope\n{ <nope; }\n' >f2
chmod +x f2

./f2
f() {
    ./f2
}
f
trap '<nope; <nope' EXIT
:
