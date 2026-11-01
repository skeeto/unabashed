declare -x ex=1
declare -p ex
e2=two
export e2
declare -p e2
export e3=three e4
declare -p e3 e4
export -n e2
declare -p e2
declare +x ex
declare -p ex
export -n e5=five
declare -p e5
typeset -x tx=t
declare -p tx
export 'bad name=1'
echo "status $?"
