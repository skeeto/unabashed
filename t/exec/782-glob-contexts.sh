# Where pathname expansion happens: command words and arrays, but not
# assignments, [[ ]], case words or here-strings; redirections use a
# single match.
cd /tmp && mkdir gc && cd gc
touch one.txt
x=*.txt; echo "$x" $x
a=(*.txt); echo "${a[@]}"
[[ *.txt == one.txt ]] && echo cond glob || echo cond literal
case *.txt in one.txt) echo case glob ;; '*.txt') echo case literal ;; esac
cat <<< *.txt
echo hello > o*.txt; cat one.txt
touch two.txt
echo x > *.txt; echo "status $?"
cat < t*.txt
declare d=*.txt; echo "$d"
export e=*.txt; echo "$e"
local_test() { local l=*.txt; echo "$l"; }; local_test
for f in *.txt; do echo "for $f"; done
set -f; echo *.txt; set +f
echo "${undefined:-*.txt}"
echo "${undefined:-"*.txt"}"
