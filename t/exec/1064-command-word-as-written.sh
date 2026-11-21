# How unset, read, printf and the declaration builtins take their
# arguments follows the command word as written, as bash decides before
# expanding: a first word that expands to nothing, or to the builtin's
# name, or that is quoted, does not count, while words "command" before
# it are passed over (for assignments only in POSIX mode).
shopt -s array_expand_once
declare -A A
k='x]'
reset() { A=(); A[x]=1; A['x]']=2; }
show() { printf '%s: ' "$1"; declare -p A; }
e=
reset; unset A[$k]; show plain
reset; $e unset A[$k]; show empty-first
reset; command unset A[$k]; show command
reset; builtin unset A[$k]; show builtin
reset; u=unset; $u A[$k]; show expanded-name
reset; "unset" A[$k]; show quoted
reset; command command unset A[$k]; show command2
reset; $e command unset A[$k]; show empty-command
reset; V=1 unset A[$k]; show tempassign
declare -a arr
k2='1]'
arr=([1]=a [2]=b); $e unset arr[$k2] 2>&1; declare -p arr
A=()
read A[$k] <<<v; show read; A=()
$e read A[$k] <<<v 2>&1; show read-empty; A=()
command read A[$k] <<<v; show read-command; A=()
builtin read A[$k] <<<v 2>&1; show read-builtin; A=()
printf -v A[$k] v; show printf; A=()
$e printf -v A[$k] v 2>&1; show printf-empty; A=()
p=printf; $p -v A[$k] v 2>&1; show printf-expanded; A=()
x='1 2'
declare b=$x; echo "b=$b"
$e declare c=$x 2>&1; echo "c=$c"
dd=declare; $dd d=$x 2>&1; echo "d=$d"
"declare" f=$x 2>&1; echo "f=$f"
command export g=$x 2>&1; echo "g=$g"
set -o posix
command export h=$x; echo "h=$h"
command command export i=$x; echo "i=$i"
