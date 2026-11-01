# sourced files as frames: FUNCNAME, BASH_SOURCE, BASH_LINENO, caller,
# DEBUG traps in them; BASH_ARGV and BASH_ARGC under extdebug
cat > sub.sh <<'XEOF'
echo "in sub: FUNCNAME=${FUNCNAME[*]-unset} #${#FUNCNAME[@]} SRC=${BASH_SOURCE[*]} LINENO=${BASH_LINENO[*]}"
sf() { echo "sf: FUNCNAME=${FUNCNAME[*]} SRC=${BASH_SOURCE[*]} LN=${BASH_LINENO[*]}"; caller 0; caller 1; caller 2; }
sf
echo "caller: $(caller 0)"
XEOF
set -- one two
echo "top: FUNCNAME=${FUNCNAME[*]-unset} SRC=${BASH_SOURCE[*]} LN=${BASH_LINENO[*]}"
. ./sub.sh
f() { . ./sub.sh a b; }
f
echo ---- extdebug
shopt -s extdebug
. ./sub.sh
f
trap 'echo "debug $LINENO ${FUNCNAME[0]-none} ${BASH_SOURCE[0]}"' DEBUG
. ./sub.sh
trap - DEBUG
echo ---- argv
g() { echo "argc=${BASH_ARGC[*]} argv=${BASH_ARGV[*]}"; . ./sub2.sh x y z; echo "after argc=${BASH_ARGC[*]} argv=${BASH_ARGV[*]}"; }
echo 'echo "s2 argc=${BASH_ARGC[*]} argv=${BASH_ARGV[*]}"' > sub2.sh
g 1 2
shopt -u extdebug
g 1 2
