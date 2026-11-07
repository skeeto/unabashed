# BASH_SOURCE of functions by where they were defined, and declare -F
# under extdebug: name, line and file
f() { echo "f: ${#BASH_SOURCE[@]} [${BASH_SOURCE[*]}]"; }
f
eval 'g() { echo "g: ${#BASH_SOURCE[@]} [${BASH_SOURCE[*]}]"; }'; g
h() {
    f
}
h
echo "top: ${#BASH_SOURCE[@]} [${BASH_SOURCE[*]}]"
printf '%s\n' '# sourced' 'k() { echo "k: [${BASH_SOURCE[*]}]"; }' 'k' > lib.sh
. ./lib.sh
k
declare -F f g h k
shopt -s extdebug
declare -F f g h k nosuch; echo "status $?"
declare -F | head -2
shopt -u extdebug
declare -F k
