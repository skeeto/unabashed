# set -e in a function runs the EXIT action still in the function, as
# bash: its FUNCNAME, BASH_SOURCE, positional parameters, traps and
# redirections stay, but at the top level, in eval, source or a command
# substitution every local variable goes (local fails, declare makes a
# global), and in a ( ) or & subshell's own code they stay. A builtin or
# function call whose command has redirections is unwound first, with
# them. Out of a function, a command substitution keeps its capture.
t='echo "exit: fn=${FUNCNAME[*]} v=$v args=$* st=$?"'
defs='v=global; f() { local v=local; trap "echo ret" RETURN; false; echo no; }; g() { f a b; }'
echo "-- top level"
bash -c "$defs"'; set -e; trap "$0; trap -p RETURN; local w=1; declare v=new; declare -p v" EXIT; g x' "$t"
echo "-- exit, for comparison"
bash -c "$defs"'; f() { local v=local; exit 3; }; trap "$0" EXIT; g x' "$t"
echo "-- eval"
bash -c "$defs"'; set -e; trap "$0" EXIT; eval "g x"' "$t"
echo "-- subshells"
v=global
f() { local v=local; false; echo no; }
g() { f a b; }
(set -e; trap "$t" EXIT; g x)
{ set -e; trap "$t" EXIT; g x; } & wait
{ set -e; trap "$t" EXIT; g x; } | cat
(set -e; trap "$t" EXIT; eval 'g x')
h() { (set -e; trap "$t" EXIT; g x); }
h y
echo "-- command substitutions"
echo "[$(set -e; trap "$t" EXIT; g x)]"
echo "[$(set -e; trap 'echo exit' EXIT; false)]"
echo "[$(set -e; trap 'echo exit' EXIT; { false; } >/dev/null)]"
echo "-- redirections"
(set -e; trap "$t" EXIT; { g x; } >/dev/null)
(set -e; trap "$t" EXIT; g x >/dev/null)
(set -e; trap "$t" EXIT; k() { g x 2>/dev/null; }; k y)
(set -e; trap "$t" EXIT; k() { eval 'g x' 2>/dev/null; }; k y)
echo 'g x' >src.sh
(set -e; trap "$t" EXIT; k() { source ./src.sh 2>/dev/null; }; k y)
bash -c "$defs"'; set -e; trap "$0" EXIT; { false; } >/dev/null' "$t"
bash -c "$defs"'; set -e; trap "$0" EXIT; { g x; } >/dev/null' "$t"
bash -c "$defs"'; set -e; trap "$0" EXIT; g x >/dev/null' "$t"
rm -f src.sh
echo done
