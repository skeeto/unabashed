shopt -s expand_aliases
alias E='esac'
echo before
f() { case x in a) ;; E; }
