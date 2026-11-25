== stdout 608
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

== stderr 0

== merged 608
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

== status 127
