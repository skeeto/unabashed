shopt -s expand_aliases
alias c='echo a # note'
f() {
    c rest; echo gone
    echo kept
}
declare -f
