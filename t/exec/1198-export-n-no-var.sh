# export -n and readonly -n take an attribute away, and as bash's
# set_var_attribute only from a variable there is: a name with none
# stays undeclared. With a value, the variable is made as usual.
export -n a; declare -p a 2>&1
readonly -n b; declare -p b 2>&1
f() { export -n c; declare -p c 2>&1; }; f
declare -p c 2>&1
export -n d=1; declare -p d
e=1; export e; export -n e; declare -p e
declare -x g; export -n g; declare -p g
declare -n r=t; export -n r; declare -p t 2>&1
h=2; readonly -n h; declare -p h
