declare -p FUNCNAME BASH_SOURCE BASH_LINENO
f() { declare -p FUNCNAME BASH_SOURCE BASH_LINENO; echo "${FUNCNAME[@]}" "$FUNCNAME" "${#FUNCNAME[@]}"; }
g() {
  f
}
g
h() { echo ${FUNCNAME[1]-unset} ${BASH_LINENO[0]} ${BASH_LINENO[1]-u} ${#FUNCNAME[@]} ${!FUNCNAME[@]} ${FUNCNAME[-1]}; }
h
k() { h; }; k
echo "${BASH_VERSINFO[@]}" "${#BASH_VERSINFO[*]}" | sed "s/${BASH_VERSINFO[5]}/HOST/"; declare -p BASH_VERSINFO GROUPS | sed "s/${BASH_VERSINFO[5]}/HOST/"
BASH_VERSINFO[0]=9; echo st=$?
FUNCNAME=x; declare -p FUNCNAME
