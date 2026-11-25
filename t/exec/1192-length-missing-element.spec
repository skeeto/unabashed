== stdout 1409
-- echo ${#map[a b]}
0
after 0
-- echo ${#x[-5]}
0
after 0
-- echo ${#x[$(echo hi)]}
0
after 0
-- x=1; unset x; echo ${#x[1/0]}
0
after 0
-- declare -a x; echo ${#x[a b]}
0
after 0
-- export x; echo ${#x[a b]}
0
after 0
-- declare -n r=q; echo ${#r[a b]}
0
after 0
-- x=; echo ${#x[a b]}
bash: line 1: a b: arithmetic syntax error in expression (error token is "b")
-- x=abc; echo ${#x[-5]}
bash: line 1: [-5]: bad array subscript
after 1
-- x=(); echo ${#x[a b]}
bash: line 1: a b: arithmetic syntax error in expression (error token is "b")
-- set -u; echo ${#map[1]}
bash: line 1: map[1]: unbound variable
-- a=(1); (( a[-5]=2 )); declare -p a
bash: line 1: a[-5]: bad array subscript
declare -a a=([0]="1")
after 0
-- a=(1); echo $(( a[-5]=2, 7 ))
bash: line 1: a[-5]: bad array subscript
7
after 0
-- a=(1); (( a[-5]++ ))
bash: line 1: a: bad array subscript
bash: line 1: a[-5]: bad array subscript
after 1
-- a=(1); let 'a[-5]+=3'
bash: line 1: a: bad array subscript
bash: line 1: a[-5]: bad array subscript
after 0
-- declare -i n; a=(1); n='a[-5]=4'; echo $n
bash: line 1: a[-5]: bad array subscript
4
after 0
-- set -e; a=(1); (( a[-5]=2 )) || echo no
bash: line 1: a[-5]: bad array subscript
-- declare -A m; (( m[]=1 ))
bash: line 1: ((: `m[]': not a valid identifier
after 0
-- declare -A m; echo $(( m[] ))
bash: line 1: m[]: bad array subscript
bash: line 1: m[]: bad array subscript
0
after 0

== stderr 0

== merged 1409
-- echo ${#map[a b]}
0
after 0
-- echo ${#x[-5]}
0
after 0
-- echo ${#x[$(echo hi)]}
0
after 0
-- x=1; unset x; echo ${#x[1/0]}
0
after 0
-- declare -a x; echo ${#x[a b]}
0
after 0
-- export x; echo ${#x[a b]}
0
after 0
-- declare -n r=q; echo ${#r[a b]}
0
after 0
-- x=; echo ${#x[a b]}
bash: line 1: a b: arithmetic syntax error in expression (error token is "b")
-- x=abc; echo ${#x[-5]}
bash: line 1: [-5]: bad array subscript
after 1
-- x=(); echo ${#x[a b]}
bash: line 1: a b: arithmetic syntax error in expression (error token is "b")
-- set -u; echo ${#map[1]}
bash: line 1: map[1]: unbound variable
-- a=(1); (( a[-5]=2 )); declare -p a
bash: line 1: a[-5]: bad array subscript
declare -a a=([0]="1")
after 0
-- a=(1); echo $(( a[-5]=2, 7 ))
bash: line 1: a[-5]: bad array subscript
7
after 0
-- a=(1); (( a[-5]++ ))
bash: line 1: a: bad array subscript
bash: line 1: a[-5]: bad array subscript
after 1
-- a=(1); let 'a[-5]+=3'
bash: line 1: a: bad array subscript
bash: line 1: a[-5]: bad array subscript
after 0
-- declare -i n; a=(1); n='a[-5]=4'; echo $n
bash: line 1: a[-5]: bad array subscript
4
after 0
-- set -e; a=(1); (( a[-5]=2 )) || echo no
bash: line 1: a[-5]: bad array subscript
-- declare -A m; (( m[]=1 ))
bash: line 1: ((: `m[]': not a valid identifier
after 0
-- declare -A m; echo $(( m[] ))
bash: line 1: m[]: bad array subscript
bash: line 1: m[]: bad array subscript
0
after 0

== status 0
