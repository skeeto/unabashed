shopt -s expand_aliases
alias nl='echo one
'
f() { nl two; }
g() {
    nl
    echo three
}
declare -f
