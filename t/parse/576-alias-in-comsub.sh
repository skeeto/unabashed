shopt -s expand_aliases
alias e='echo'
alias c2='cat <<'
alias lp='{ echo'
f() {
    x=$(e hi)
    y=`e there`
    z=${ e funsub; }
    c2 E
body
E
    echo $(c2 F
fb
F
)
    lp grp; }
}
declare -f
