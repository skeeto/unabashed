# =~ with back-references after repetition: exponentially many paths,
# searched in polynomial time
a=$(printf '%300s' '' | tr ' ' a)
m() { echo "$1 $2 ${#BASH_REMATCH[0]} ${#BASH_REMATCH[1]} ${#BASH_REMATCH[2]}"; }
re='((a|aa)*)\1\1c'
[[ ${a}c =~ $re ]]; m 1 $?
[[ $a =~ $re ]]; m 2 $?
re='((a|aa)*)\1\1'
[[ $a =~ $re ]]; m 3 $?
[[ ${a:0:299} =~ $re ]]; m 4 $?
re='^(a|aa)+\1$'
[[ ${a:0:100}b =~ $re ]]; m 5 $?
[[ ${a:0:100} =~ $re ]]; m 6 $?
re='(a*)*\1b'
[[ ${a:0:60} =~ $re ]]; m 7 $?
re='(a|b)*\1\1x'
[[ $(printf '%100s' '' | sed 's/ /ab/g') =~ $re ]]; m 8 $?
re='(a|aa)*\1c'
[[ ${a:0:60}c =~ $re ]]; m 9 $?
