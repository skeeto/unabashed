shopt -s expand_aliases
alias h='cat <<END
inside
END
'
alias s='cat <<E
first'
alias hc='echo $(cat <<C
x
C
)'
f() {
    h
    echo after
}
g() {
    s rest
E
    echo next
}
k() { hc; }
declare -f
