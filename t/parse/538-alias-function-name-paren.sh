shopt -s expand_aliases
alias nm='realname' sp='name '
nm() { echo one; }
sp () { echo two; }
declare -f
