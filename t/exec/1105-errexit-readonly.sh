# Under set -e, bash's report_error ends the shell after every error it
# reports that way, a readonly variable assigned above all, even in a
# condition or an && / || list, where a failing command would not: with
# $? from before it, or else 1, and the EXIT action where exit's runs.
# eval, source and command run as a condition turn this off for all
# they run, a function does not; a ( ) subshell ends itself only, and a
# command substitution does only with set -e of its own.
p='trap "echo \"exit \$? [\${FUNCNAME[*]}] \$l\"" EXIT; set -e; readonly r=1 u;'
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$p $c; echo after" 2>&1; echo "status $?"
done <<'EOF'
if read r <<<x; then :; fi
read r <<<x || :
! read r <<<x
(exit 3) || read r <<<x
f() { local l=5; read r <<<x; }; if f; then :; fi
if printf -v r x; then :; fi
if getopts a: r -a x; then :; fi
readonly OPTARG; getopts a o -a || :
if for r in 1; do :; done; then :; fi
select r in a; do break; done <<<1 || :
if ((r=2)); then :; fi
let r=1 r+=1 || :
for ((r=0;0;)); do :; done || :
if : ${u:=x}; then :; fi
false || r=2 :
if x=1 r=2 y=3 true; then :; fi
f() { :; }; r=2 f
r=2 cat </dev/null
export r=3 || :
if readonly r=3; then :; fi
declare -n n=r; if read n <<<x; then :; fi
if mapfile r </dev/null; then :; fi
if r+=(1); then :; fi
if declare r=2; then :; fi
if unset r; then :; fi
if eval 'read r <<<x; echo in'; then :; fi
if command read r <<<x; then :; fi
if builtin read r <<<x; then :; fi
echo 'read r <<<x; echo in' >src.sh; if . ./src.sh; then :; fi
f() { eval 'read r <<<x'; echo inf; }; if f; then :; fi
if eval 'x=$(set -e; read r <<<x; echo in); echo "<$x>"'; then :; fi
if eval '( read r <<<x; echo in )'; then :; fi
( read r <<<x; echo in ) || :
{ read r <<<x; echo in; } || :
x=$(read r <<<x; echo in) || :; echo "<$x>"
shopt -s inherit_errexit; x=$(if read r <<<x; then :; fi; echo in) || :; echo "<$x>"
trap 'if read r <<<x; then :; fi; echo trap' USR1; kill -USR1 $$
trap 'echo trap; read r <<<x' USR1; if eval 'kill -USR1 $$'; then :; fi
readonly _; if true; then :; fi
EOF
rm -f src.sh
