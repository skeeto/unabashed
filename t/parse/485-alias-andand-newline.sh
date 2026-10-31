shopt -s expand_aliases
alias aa='true &&' ab='true ||
'
f() {
    aa
    echo x
    ab echo y
}
declare -f
