# [[ =~ ]]: status and BASH_REMATCH
[[ abc =~ b ]]; echo "$? ${BASH_REMATCH[*]}"
[[ abc =~ x ]]; echo "$? ${#BASH_REMATCH[@]}"; declare -p BASH_REMATCH
[[ xaby =~ (a)(b)(c)? ]]; echo "$? ${#BASH_REMATCH[@]}"; declare -p BASH_REMATCH
[[ "foo bar" =~ ^([a-z]+)\ ([a-z]+)$ ]] && echo "${BASH_REMATCH[2]}-${BASH_REMATCH[1]}"
[[ aaa =~ (a)* ]]; declare -p BASH_REMATCH
[[ abcd =~ (a|ab)(c|bcd)(d*) ]]; declare -p BASH_REMATCH
[[ "" =~ ^$ ]]; echo "$? ${#BASH_REMATCH[@]} <${BASH_REMATCH[0]}>"
[[ x =~ "" ]]; echo "$?"
s='Dog 01 is Wiggles'
[[ $s =~ ^Dog\ ([0-9]+)\ is\ (.*)$ ]] && echo "${BASH_REMATCH[2]} ${BASH_REMATCH[1]}"
BASH_REMATCH=(stale values here); [[ q =~ q ]]; declare -p BASH_REMATCH
BASH_REMATCH=scalar; [[ q =~ z ]]; declare -p BASH_REMATCH
f() { local BASH_REMATCH; [[ ab =~ (b) ]]; declare -p BASH_REMATCH; }
unset BASH_REMATCH; f; declare -p BASH_REMATCH
g() { [[ cd =~ (d) ]]; }; g; declare -p BASH_REMATCH
declare -A BASH_REMATCH; [[ a =~ a ]]; echo "status $?"; unset BASH_REMATCH
readonly BASH_REMATCH; [[ a =~ a ]]; echo "status $?"
echo done
