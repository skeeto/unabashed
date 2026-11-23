# BASH_MONOSECONDS: whole seconds of a monotonic clock, computed when
# read, as bash 5.3's (CLOCK_MONOTONIC)
set | grep -c ^BASH_MONOSECONDS
declare -p | grep -c BASH_MONOSECONDS
x=$BASH_MONOSECONDS; [[ $x =~ ^[0-9]+$ ]] && echo digits
set | grep ^BASH_MONOSECONDS | sed 's/=[0-9]*$/=N/'
declare -p BASH_MONOSECONDS | sed 's/"[0-9]*"/N/'
BASH_MONOSECONDS=5; [ "$BASH_MONOSECONDS" != 5 ] && echo dynamic
y=$BASH_MONOSECONDS; [ $((y - x)) -ge 0 ] && echo forward
export BASH_MONOSECONDS; printenv BASH_MONOSECONDS | sed 's/^[0-9]*$/N/'
BASH_MONOSECONDS=7; printenv BASH_MONOSECONDS
(BASH_MONOSECONDS[1]=a; declare -p BASH_MONOSECONDS | sed 's/"[0-9]*" \[1\]/N [1]/')
unset BASH_MONOSECONDS; echo "[$BASH_MONOSECONDS]"
env BASH_MONOSECONDS=3 bash -c 'printenv BASH_MONOSECONDS; echo "st $?"
[ "$BASH_MONOSECONDS" != 3 ] && echo dropped'
