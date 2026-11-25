== stdout 1347
-- export x; echo "[${!x}]"
[]
after 0
-- declare x; echo "[${!x}]"
[]
after 0
-- declare -a x; echo "[${!x}]"
[]
after 0
-- readonly x; echo "[${!x}]"
[]
after 0
-- f() { local x; echo "[${!x}]"; }; f
[]
after 0
-- f() { local x=1; unset x; echo "[${!x}]"; }; f
[]
after 0
-- x=1; unset x; echo "[${!x}]"
bash: line 1: x: invalid indirect expansion
after 1
-- export x; echo "[${!x-d}] [${!x:+a}] [${!x@Q}] [${!x#a}]"
[d] [] [] []
after 0
-- declare -i x; echo "[${!x:-d}]"
[d]
after 0
-- set -u; export x; echo "[${!x}]"
bash: line 1: !x: unbound variable
-- export x; echo "[${!x?}]"
bash: line 1: !x: parameter not set
-- export x; echo "[${!x:=a}]"
bash: line 1: !x: invalid indirect expansion
after 1
-- export x; echo "[${!x=a}]"
bash: line 1: !x: invalid indirect expansion
after 1
-- echo "[${!1:=a}]"
bash: line 1: !1: invalid indirect expansion
after 1
-- a=(); echo "[${!a[0]:=a}]"
bash: line 1: !a[0]: invalid indirect expansion
after 1
-- x=y; echo "[${!x:=a}] $y"
[a] a
after 0
-- declare -n r; echo "[${!r}]"
bash: line 1: r: invalid indirect expansion
after 1
-- declare -n r; echo "[${!r-d}]"
bash: line 1: r: invalid indirect expansion
after 1
-- declare -n r s=r; echo "[${!s}]"
bash: line 1: s: invalid indirect expansion
after 1
-- declare -n r=x; echo "[${!r}]"
[x]
after 0
-- declare -n r=s s=t; echo "[${!r}]"
[t]
after 0

== stderr 0

== merged 1347
-- export x; echo "[${!x}]"
[]
after 0
-- declare x; echo "[${!x}]"
[]
after 0
-- declare -a x; echo "[${!x}]"
[]
after 0
-- readonly x; echo "[${!x}]"
[]
after 0
-- f() { local x; echo "[${!x}]"; }; f
[]
after 0
-- f() { local x=1; unset x; echo "[${!x}]"; }; f
[]
after 0
-- x=1; unset x; echo "[${!x}]"
bash: line 1: x: invalid indirect expansion
after 1
-- export x; echo "[${!x-d}] [${!x:+a}] [${!x@Q}] [${!x#a}]"
[d] [] [] []
after 0
-- declare -i x; echo "[${!x:-d}]"
[d]
after 0
-- set -u; export x; echo "[${!x}]"
bash: line 1: !x: unbound variable
-- export x; echo "[${!x?}]"
bash: line 1: !x: parameter not set
-- export x; echo "[${!x:=a}]"
bash: line 1: !x: invalid indirect expansion
after 1
-- export x; echo "[${!x=a}]"
bash: line 1: !x: invalid indirect expansion
after 1
-- echo "[${!1:=a}]"
bash: line 1: !1: invalid indirect expansion
after 1
-- a=(); echo "[${!a[0]:=a}]"
bash: line 1: !a[0]: invalid indirect expansion
after 1
-- x=y; echo "[${!x:=a}] $y"
[a] a
after 0
-- declare -n r; echo "[${!r}]"
bash: line 1: r: invalid indirect expansion
after 1
-- declare -n r; echo "[${!r-d}]"
bash: line 1: r: invalid indirect expansion
after 1
-- declare -n r s=r; echo "[${!s}]"
bash: line 1: s: invalid indirect expansion
after 1
-- declare -n r=x; echo "[${!r}]"
[x]
after 0
-- declare -n r=s s=t; echo "[${!r}]"
[t]
after 0

== status 0
