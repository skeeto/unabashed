== stdout 3840
-- if read r <<<x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- read r <<<x || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- ! read r <<<x
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- (exit 3) || read r <<<x
bash: line 1: r: readonly variable
exit 3 [] 
status 3
-- f() { local l=5; read r <<<x; }; if f; then :; fi
bash: line 1: r: readonly variable
exit 1 [f] 5
status 1
-- if printf -v r x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if getopts a: r -a x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- readonly OPTARG; getopts a o -a || :
bash: line 1: OPTARG: readonly variable
exit 1 [] 
status 1
-- if for r in 1; do :; done; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- select r in a; do break; done <<<1 || :
1) a
#? bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if ((r=2)); then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- let r=1 r+=1 || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- for ((r=0;0;)); do :; done || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if : ${u:=x}; then :; fi
bash: line 1: u: readonly variable
exit 1 [] 
status 1
-- false || r=2 :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if x=1 r=2 y=3 true; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- f() { :; }; r=2 f
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- r=2 cat </dev/null
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- export r=3 || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if readonly r=3; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- declare -n n=r; if read n <<<x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if mapfile r </dev/null; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if r+=(1); then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if declare r=2; then :; fi
bash: line 1: declare: r: readonly variable
after
exit 0 [] 
status 0
-- if unset r; then :; fi
bash: line 1: unset: r: cannot unset: readonly variable
after
exit 0 [] 
status 0
-- if eval 'read r <<<x; echo in'; then :; fi
bash: line 1: r: readonly variable
in
after
exit 0 [] 
status 0
-- if command read r <<<x; then :; fi
bash: line 1: r: readonly variable
after
exit 0 [] 
status 0
-- if builtin read r <<<x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- echo 'read r <<<x; echo in' >src.sh; if . ./src.sh; then :; fi
./src.sh: line 1: r: readonly variable
in
after
exit 0 [] 
status 0
-- f() { eval 'read r <<<x'; echo inf; }; if f; then :; fi
bash: line 1: r: readonly variable
inf
after
exit 0 [] 
status 0
-- if eval 'x=$(set -e; read r <<<x; echo in); echo "<$x>"'; then :; fi
bash: line 1: r: readonly variable
<>
after
exit 0 [] 
status 0
-- if eval '( read r <<<x; echo in )'; then :; fi
bash: line 1: r: readonly variable
in
after
exit 0 [] 
status 0
-- ( read r <<<x; echo in ) || :
bash: line 1: r: readonly variable
after
exit 0 [] 
status 0
-- { read r <<<x; echo in; } || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- x=$(read r <<<x; echo in) || :; echo "<$x>"
bash: line 1: r: readonly variable
<in>
after
exit 0 [] 
status 0
-- shopt -s inherit_errexit; x=$(if read r <<<x; then :; fi; echo in) || :; echo "<$x>"
bash: line 1: r: readonly variable
<>
after
exit 0 [] 
status 0
-- trap 'if read r <<<x; then :; fi; echo trap' USR1; kill -USR1 $$
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- trap 'echo trap; read r <<<x' USR1; if eval 'kill -USR1 $$'; then :; fi
trap
bash: line 1: r: readonly variable
after
exit 0 [] 
status 0
-- readonly _; if true; then :; fi
bash: line 1: _: readonly variable
exit 1 [] 
bash: line 1: _: readonly variable
status 1

== stderr 0

== merged 3840
-- if read r <<<x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- read r <<<x || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- ! read r <<<x
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- (exit 3) || read r <<<x
bash: line 1: r: readonly variable
exit 3 [] 
status 3
-- f() { local l=5; read r <<<x; }; if f; then :; fi
bash: line 1: r: readonly variable
exit 1 [f] 5
status 1
-- if printf -v r x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if getopts a: r -a x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- readonly OPTARG; getopts a o -a || :
bash: line 1: OPTARG: readonly variable
exit 1 [] 
status 1
-- if for r in 1; do :; done; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- select r in a; do break; done <<<1 || :
1) a
#? bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if ((r=2)); then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- let r=1 r+=1 || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- for ((r=0;0;)); do :; done || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if : ${u:=x}; then :; fi
bash: line 1: u: readonly variable
exit 1 [] 
status 1
-- false || r=2 :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if x=1 r=2 y=3 true; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- f() { :; }; r=2 f
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- r=2 cat </dev/null
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- export r=3 || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if readonly r=3; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- declare -n n=r; if read n <<<x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if mapfile r </dev/null; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if r+=(1); then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- if declare r=2; then :; fi
bash: line 1: declare: r: readonly variable
after
exit 0 [] 
status 0
-- if unset r; then :; fi
bash: line 1: unset: r: cannot unset: readonly variable
after
exit 0 [] 
status 0
-- if eval 'read r <<<x; echo in'; then :; fi
bash: line 1: r: readonly variable
in
after
exit 0 [] 
status 0
-- if command read r <<<x; then :; fi
bash: line 1: r: readonly variable
after
exit 0 [] 
status 0
-- if builtin read r <<<x; then :; fi
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- echo 'read r <<<x; echo in' >src.sh; if . ./src.sh; then :; fi
./src.sh: line 1: r: readonly variable
in
after
exit 0 [] 
status 0
-- f() { eval 'read r <<<x'; echo inf; }; if f; then :; fi
bash: line 1: r: readonly variable
inf
after
exit 0 [] 
status 0
-- if eval 'x=$(set -e; read r <<<x; echo in); echo "<$x>"'; then :; fi
bash: line 1: r: readonly variable
<>
after
exit 0 [] 
status 0
-- if eval '( read r <<<x; echo in )'; then :; fi
bash: line 1: r: readonly variable
in
after
exit 0 [] 
status 0
-- ( read r <<<x; echo in ) || :
bash: line 1: r: readonly variable
after
exit 0 [] 
status 0
-- { read r <<<x; echo in; } || :
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- x=$(read r <<<x; echo in) || :; echo "<$x>"
bash: line 1: r: readonly variable
<in>
after
exit 0 [] 
status 0
-- shopt -s inherit_errexit; x=$(if read r <<<x; then :; fi; echo in) || :; echo "<$x>"
bash: line 1: r: readonly variable
<>
after
exit 0 [] 
status 0
-- trap 'if read r <<<x; then :; fi; echo trap' USR1; kill -USR1 $$
bash: line 1: r: readonly variable
exit 1 [] 
status 1
-- trap 'echo trap; read r <<<x' USR1; if eval 'kill -USR1 $$'; then :; fi
trap
bash: line 1: r: readonly variable
after
exit 0 [] 
status 0
-- readonly _; if true; then :; fi
bash: line 1: _: readonly variable
exit 1 [] 
bash: line 1: _: readonly variable
status 1

== status 0
