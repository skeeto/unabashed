# A plain name=(...) makes the array, or converts the scalar there to
# one, before its words expand, as bash's find_or_make_array_variable:
# the words see the array, and an expansion error leaves it made.
a=( "$(declare -p a)" ); declare -p a
x=s; x=( "$(declare -p x)" ); declare -p x
x=s; x+=( "$(declare -p x)" ); declare -p x
declare -n r=t; r=( "$(declare -p t)" ); declare -p t
f() { local l; l=( "$(declare -p l)" ); declare -p l; }; f
b=( "${b=1}" ); declare -p b
arr+=( a $((1/0)) )
declare -p arr
y=1; y=( "${!u}" )
declare -p y
z=; z+=( a $((1/0)) )
declare -p z
g() { w=( "${!u}" ); }; g
declare -p w
declare -i n=5; n=( "${!u}" )
declare -p n
declare -A m=([k]=v); m=( "${!u}" )
declare -p m
v=( "${!u}" ) :
declare -p v 2>&1
