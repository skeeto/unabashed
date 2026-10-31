shopt -s expand_aliases
alias hd='cat <<E'
alias cs='echo $(cat <<F'
alias p='cat |'
f() {
    hd
body $x
E
    cs
fbody
F
)
    p hd
pipe
E
    echo $(hd
inner
E
)
}
declare -f
