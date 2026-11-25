== stdout 2330
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
-- echo ${!r}
bash: line 1: r: invalid indirect expansion
after 1
status 0
-- r=1a; echo ${!r}
bash: line 1: 1a: invalid variable name
after 1
status 0
-- declare -n r; echo ${!r}
bash: line 1: r: invalid indirect expansion
after 1
status 0
-- echo ${!x:=v}
bash: line 1: x: invalid indirect expansion
after 1
status 0
-- echo ${1=x}
bash: line 1: $1: cannot assign in this way
after 1
status 0
-- shopt -s failglob; echo /nope*
bash: line 1: no match: /nope*
after 1
status 0
-- x=(1); echo ${#x[-5]}
bash: line 1: [-5]: bad array subscript
after 1
status 0
-- x=abc; echo ${x:1:-5}
bash: line 1: -5: substring expression < 0
after 1
status 0
-- echo ${x!}
bash: line 1: ${x!}: bad substitution
status 127
-- echo ${arr[-1]:=a}
bash: line 1: arr: bad array subscript
bash: line 1: arr[-1]: bad array subscript
status 127
-- set -u; echo ${a[@]}${a[1]}
bash: line 1: a[1]: unbound variable
status 127
-- echo $((1/0))
bash: line 1: 1/0: division by 0 (error token is "0")
status 127

== stderr 0

== merged 2330
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
-- echo ${!r}
bash: line 1: r: invalid indirect expansion
after 1
status 0
-- r=1a; echo ${!r}
bash: line 1: 1a: invalid variable name
after 1
status 0
-- declare -n r; echo ${!r}
bash: line 1: r: invalid indirect expansion
after 1
status 0
-- echo ${!x:=v}
bash: line 1: x: invalid indirect expansion
after 1
status 0
-- echo ${1=x}
bash: line 1: $1: cannot assign in this way
after 1
status 0
-- shopt -s failglob; echo /nope*
bash: line 1: no match: /nope*
after 1
status 0
-- x=(1); echo ${#x[-5]}
bash: line 1: [-5]: bad array subscript
after 1
status 0
-- x=abc; echo ${x:1:-5}
bash: line 1: -5: substring expression < 0
after 1
status 0
-- echo ${x!}
bash: line 1: ${x!}: bad substitution
status 127
-- echo ${arr[-1]:=a}
bash: line 1: arr: bad array subscript
bash: line 1: arr[-1]: bad array subscript
status 127
-- set -u; echo ${a[@]}${a[1]}
bash: line 1: a[1]: unbound variable
status 127
-- echo $((1/0))
bash: line 1: 1/0: division by 0 (error token is "0")
status 127

== status 0
