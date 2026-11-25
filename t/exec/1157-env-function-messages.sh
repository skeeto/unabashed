# A function imported from the environment comes from "environment",
# which its messages name, and its text's first line is line 0: a
# message there names no line, except a builtin's.
f() {
  nosuch
  cat <nope
  echo "L=$LINENO S=${BASH_SOURCE[*]} F=${FUNCNAME[*]} BL=${BASH_LINENO[*]}"
  : ${x?unset here}
}
g() { echo $((1/0)); }
h() { declare -p nonex; arr[x]=1 true; caller; echo "L=$LINENO"; }
export -f f g h
bash -c 'f; echo "st=$?"'
bash -c 'g; echo "st=$?"'
bash -c 'h; echo "st=$?"'
bash -c 'k() { f; }
k'
bash -c 'shopt -s extdebug; declare -F f g h'
PS4='+$LINENO: ' bash -xc 'h' 2>&1
# a function defined while one runs comes from there too
m() { n() { nosuch2; }; n; echo "S=${BASH_SOURCE[*]}"; }
export -f m
bash -c 'm; n'
