# set -o history gives HISTSIZE and HISTFILESIZE their defaults: a
# nameref to an element whose subscript runs a command substitution
# does not take the shell down. (bash assigns a variable of the name
# instead, unabashed through the nameref: only the outcome is shown.)
HISTFILE=$PWD/hist
declare -n HISTFILESIZE='a[$(echo 1)/1]'
set -o history
echo "1 $?"
set +o history
unset HISTSIZE
declare -n HISTSIZE='b[$(echo 2)/1]'
set -o history
echo "2 $?"
echo survived
