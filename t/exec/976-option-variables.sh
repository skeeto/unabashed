# set -o posix and set -o ignoreeof assign POSIXLY_CORRECT and IGNOREEOF,
# an assignment whose memory grows with the user's text: an integer
# variable's expression, or a nameref's subscript, which may run commands
exec 2>&1
e=$(printf '1+%.0s' {1..200})1
(declare -i POSIXLY_CORRECT; b=(); y="b[$e]=5"; set -o posix; echo $POSIXLY_CORRECT ${!b[@]})
(declare -i POSIXLY_CORRECT; b=(); y='b[$(echo 4)]=6'; set -o posix; declare -p b)
(declare -A a=(); declare -n IGNOREEOF="a[$e]"; set -o ignoreeof; echo ${!a[@]} ${a[@]})
(c=(); declare -n IGNOREEOF='c[$(echo 3)]'; set -o ignoreeof; declare -p c)
echo done
