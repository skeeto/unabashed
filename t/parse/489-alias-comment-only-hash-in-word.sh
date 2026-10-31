shopt -s expand_aliases
alias c2='# only a comment'
alias c3='echo x#y'
f() {
    c2 stuff; echo gone
    c3 z#w
    echo kept
}
declare -f
