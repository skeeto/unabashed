echo start
[[ "1 2" -eq 1 ]]; echo "space $?"
[[ 1+ -eq 1 || a ]]; echo "or $?"
[[ ! 1+ -eq 1 ]]; echo "not $?"
i=0
[[ 1+ -eq i++ ]]; echo "rhs-skipped i=$i"
[[ i++ -eq 1+ ]]; echo "lhs-done i=$i"
[[ 1/0 -eq 0 && $(echo reached >&2) ]]; echo "and $?"
if [[ 2#9 -gt 0 ]]; then echo then; else echo "else $?"; fi
echo next
