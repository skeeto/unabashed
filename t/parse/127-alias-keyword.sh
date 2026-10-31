shopt -s expand_aliases
alias when='if'
alias begin='{'
alias end='}'
f() { when true; then begin echo x; end; fi; }
declare -f
