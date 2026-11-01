isyes() { [[ $1 == [yY]* ]]; }
isyes yes; echo "yes $?"
isyes Yup; echo "Yup $?"
isyes no; echo "no $?"
ispos() { (( $1 > 0 )); }
ispos 3 && echo "3 positive"
ispos -3 || echo "-3 not positive"
isfile() { test -f "$1"; }
: > /tmp/f
isfile /tmp/f; echo "file $?"
isfile /tmp/g; echo "nofile $?"
! [[ a == b ]]; echo "bang $?"
! test a = a; echo "bang test $?"
[[ a == a ]] && echo and || echo or
[[ a == b ]] && echo and || echo or
x=$( [[ 1 -eq 1 ]]; echo $? ); echo "cmdsub $x"
( (( 0 )) ); echo "subshell $?"
{ [ 1 -eq 1 ]; }; echo "group $?"
if [ a = b ]; then :; elif [[ c == c ]]; then echo elif; fi
case $( (( 2 + 2 == 4 )) && echo ok ) in ok) echo case-ok;; esac
