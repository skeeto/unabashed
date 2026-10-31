shopt -s expand_aliases
alias nl='echo one
'
echo before
f() { nl; }
