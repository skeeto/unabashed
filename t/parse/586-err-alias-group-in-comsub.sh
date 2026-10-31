shopt -s expand_aliases
alias lp='{ echo'
f() {
    echo $(lp in; })
}
declare -f
