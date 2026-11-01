== stdout 216
x=5
x=20
x+=10 -> 30
x+=2*3 -> 36
x=y -> 7
x=y*2 -> 14
x=abc -> 0
x=abc (set) -> 9
hex -> 31
oct -> 8
empty -> '0'
z=9
plain s=53
int n=8
m=8 n=9
local l=16
l=17
w=1 v=2
declare -i x="0"
declare -i z="9"
unint x=1+1

== stderr 0

== merged 216
x=5
x=20
x+=10 -> 30
x+=2*3 -> 36
x=y -> 7
x=y*2 -> 14
x=abc -> 0
x=abc (set) -> 9
hex -> 31
oct -> 8
empty -> '0'
z=9
plain s=53
int n=8
m=8 n=9
local l=16
l=17
w=1 v=2
declare -i x="0"
declare -i z="9"
unint x=1+1

== status 0
