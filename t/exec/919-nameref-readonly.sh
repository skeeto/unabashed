# declare through a nameref to a readonly variable: an assignment
# names the nameref, attributes alone or a failed local name its
# target; a failed assignment to a readonly nameref without a value
# leaves it naming "" at top level, where +r then does nothing
exec 2>&1
k() { typeset -n q=z; typeset -r q=1; typeset q=2; echo "s=$?"; declare -p q z; }; k
typeset -n r=w; typeset -r r; declare -p r w
typeset r=x; echo "s=$?"; declare -p r w
typeset +r r; echo "s=$?"; declare -p r w
typeset -rn ro=v; typeset ro=x; echo "s=$?"; declare -p ro v
typeset +r ro; echo "s=$?"; declare -p ro v
typeset -i r; echo "s=$?"; typeset -a r; echo "s=$?"; typeset -x r=3; echo "s=$?"
readonly gl=1; m() { local -n q=gl; local q=2; echo "s=$?"; typeset -i q; echo "s=$?"; }; m
n() { typeset -n q=z; typeset -r q=1; typeset -i q; echo "s=$?"; typeset +r q; echo "s=$?"; }; n
typeset -i r=4; echo "s=$?"; n2() { typeset -n q=z; typeset -r q=1; typeset -l q=A; echo "s=$?"; }; n2
typeset -n -r rr; declare -p rr
typeset rr=x; echo "s=$?"; declare -p rr
typeset +r rr; echo "s=$?"; declare -p rr
typeset -n -r rr2; typeset +r rr2; echo "s=$?"; declare -p rr2
f() { typeset -n -r rr3; typeset rr3=x; echo "s=$?"; typeset +r rr3; echo "s=$?"; declare -p rr3; }; f
