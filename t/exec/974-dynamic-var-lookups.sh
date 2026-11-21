# -v tests a dynamic variable, and the shell reads its own variables
# (IFS, GLOBSORT, BASH_COMPAT, the history and locale variables, ...)
# through a nameref to a dynamic variable, or to an element, which
# reads as unset without its subscript expanded
exec 2>&1
[[ -v RANDOM ]]; echo rc=$?
test -v EPOCHREALTIME; echo rc=$?
[ -v SECONDS ]; echo rc=$?
[[ -v FUNCNAME ]]; echo rc=$?
f() { [[ -v FUNCNAME ]]; echo f rc=$?; }; f
a=(x y); declare -n r='a[1]'; [[ -v r ]]; echo r rc=$?
declare -n t='a[$(echo no >&2; echo 1)]'; test -v t; echo t rc=$?
declare -n s=BASHPID; [[ -v s ]]; echo s rc=$?

(declare -n IFS=SECONDS; x="a b"; echo $x; echo alive)
(declare -n IFS=BASH_SUBSHELL; x=a1b; echo $x; set -- p q; echo "$*"; read u v <<<"c1d"; echo "$v")
(a=(x ,); declare -n IFS='a[1]'; x="a,b c"; printf '<%s>' $x; echo; set -- p q; echo "$*")

touch b a
(declare -n GLOBSORT=BASH_SUBSHELL; echo *)
(g=(x -name); declare -n GLOBSORT='g[1]'; echo *)
(c=(x 51); declare -n BASH_COMPAT='c[1]'; b=(1 2); unset 'b[@]'; declare -p b)

# A function's locals going away set the variables back
names='LANG HOSTFILE HISTSIZE HISTCONTROL histchars HISTTIMEFORMAT HISTFILESIZE GLOBIGNORE'
f() { local $names; }
for target in BASH_SUBSHELL FUNCNAME 'a[1]'; do
    (for n in $names; do declare -n $n="$target"; done; f; echo "$target" back; echo *)
done
