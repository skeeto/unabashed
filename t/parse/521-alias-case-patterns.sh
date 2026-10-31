shopt -s expand_aliases
alias x='echo X'
f() { case x in x) x;; (x) x;; x|x) x;; esac; }
declare -f
