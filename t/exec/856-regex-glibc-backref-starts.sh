# =~ where glibc's back-reference spans and its search decide: an
# empty alternative next to an anchored group, a newline the match
# consumed before ^, and UTF-8 matches starting inside a character
m() { echo "$1 $2 ${#BASH_REMATCH[@]} <${BASH_REMATCH[0]}> <${BASH_REMATCH[1]}> <${BASH_REMATCH[2]}>"; }
re='(|^())\2'
[[ a =~ $re ]]; m 1 $?
re='(y|^(a|))\2'
[[ a =~ $re ]]; m 2 $?
re='(|^())\1a'
[[ aab =~ $re ]]; m 3 $?
re='(a|\B)\1(.)'
[[ '`ab' =~ $re ]]; m 4 $?
re='(a|\b|())\1\|'
[[ 'c|' =~ $re ]]; m 5 $?
re=$'.^(a)\\1'
[[ $'\naa' =~ $re ]]; m 6 $?
re='(.)^\1'
[[ $'\n\n' =~ $re ]]; m 7 $?
re='(|a)$\1|(a)\2'
[[ aa =~ $re ]]; m 8 $?
LC_ALL=C.UTF-8
re=$'\xa5x[^a]'
[[ åxy =~ $re ]]; echo 9 $?
re=$'\xa5x[y]'
[[ åxy =~ $re ]]; echo 10 $? ${#BASH_REMATCH[0]}
re=$'\xa5'
[[ å =~ $re ]]; echo 11 $?
shopt -s nocasematch
[[ å =~ $re ]]; echo 12 $?
