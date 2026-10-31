shopt -s expand_aliases
alias run='nice '
alias ls='ls -l'
alias noexp='nice'
f() { run ls x; noexp ls y; }
declare -f
