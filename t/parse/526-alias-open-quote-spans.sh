shopt -s expand_aliases
alias q='echo "a'
alias s="echo 'b"
f() { q b"; q
c"; s d'; }
declare -f
