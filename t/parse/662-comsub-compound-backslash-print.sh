f() { echo $(y=(a\\
b)) <(z=(\$x)); }
declare -f
