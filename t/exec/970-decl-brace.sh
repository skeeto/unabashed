# A declaration builtin's assignment that brace expansion changed is an
# assignment no longer: it is split and globbed, with no tilde after =
HOME=/home/u
touch X=a X=b
export X=$(echo a{,}[]){,}; echo "st $?"; declare -p X
declare X=$(echo "a b"){,}; declare -p X b
declare X=~{,}; declare -p X
declare X{,}=*; declare -p X
declare X=X=*{,}; declare -p X
declare X=a{b,c}; declare -p X
declare X="a b"{,}; declare -p X
declare X={a}; declare -p X
declare X=~/a; declare -p X
f() { local Y=$(echo c d){,}; declare -p Y d; }; f
(set -f; declare X{,}=*; declare -p X)
