== stdout 904
declare -a MAPFILE=([0]=$'a\n' [1]=$'b\n')
declare -a x=([0]="a" [1]="b")
declare -a a=([0]="one" [1]="two" [2]="" [3]="four")
declare -a a=([0]=$'3\n' [1]=$'4\n')
declare -a a=([0]=$'1\n' [1]=$'2\n')
declare -a a=([0]="x" [1]="p" [2]="q" [3]="w")
declare -a a=([0]="p" [1]="q")
declare -a s=([0]="scalar" [1]=$'x\n')
declare -a s=([0]=$'x\n')
declare -a a=([0]="a" [1]="c")
declare -a a=([0]="a" [1]=$'b\nc' [2]="d")
declare -a a=([0]="a" [1]=$'b\nc' [2]="d")
declare -a a=([0]="a" [1]="b" [2]="")
declare -a a=([0]=$'line\r' [1]="last")
declare -a a=([0]="a" [1]="b" [2]="c" [3]=$'\n')
declare -a a=()
declare -a b=([0]="from 3" [1]="x")
declare -ai ints=([0]="3" [1]="12")
declare -au up=([0]="AB" [1]="CD")
declare -a loc=([0]="l1" [1]="l2")
bash: line 21: declare: loc: not found
0 x from 3
20000 20000
6
7
8
9
10
declare -a a=([0]="1" [1]="2" [2]="3")
declare -a b=([0]="4" [1]="5")
4
5
6
7
8
9
10

== stderr 0

== merged 904
declare -a MAPFILE=([0]=$'a\n' [1]=$'b\n')
declare -a x=([0]="a" [1]="b")
declare -a a=([0]="one" [1]="two" [2]="" [3]="four")
declare -a a=([0]=$'3\n' [1]=$'4\n')
declare -a a=([0]=$'1\n' [1]=$'2\n')
declare -a a=([0]="x" [1]="p" [2]="q" [3]="w")
declare -a a=([0]="p" [1]="q")
declare -a s=([0]="scalar" [1]=$'x\n')
declare -a s=([0]=$'x\n')
declare -a a=([0]="a" [1]="c")
declare -a a=([0]="a" [1]=$'b\nc' [2]="d")
declare -a a=([0]="a" [1]=$'b\nc' [2]="d")
declare -a a=([0]="a" [1]="b" [2]="")
declare -a a=([0]=$'line\r' [1]="last")
declare -a a=([0]="a" [1]="b" [2]="c" [3]=$'\n')
declare -a a=()
declare -a b=([0]="from 3" [1]="x")
declare -ai ints=([0]="3" [1]="12")
declare -au up=([0]="AB" [1]="CD")
declare -a loc=([0]="l1" [1]="l2")
bash: line 21: declare: loc: not found
0 x from 3
20000 20000
6
7
8
9
10
declare -a a=([0]="1" [1]="2" [2]="3")
declare -a b=([0]="4" [1]="5")
4
5
6
7
8
9
10

== status 0
