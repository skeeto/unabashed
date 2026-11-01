zeta() { :; }
alpha() { :; }
mid() { :; }
declare -F
declare -F alpha
echo "status $?"
declare -F nosuch
echo "status $?"
declare -F alpha nosuch zeta
echo "status $?"
declare -f nosuch
echo "status $?"
typeset -F mid
