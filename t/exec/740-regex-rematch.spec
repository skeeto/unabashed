== stdout 399
0 b
1 0
declare -a BASH_REMATCH=()
0 4
declare -a BASH_REMATCH=([0]="ab" [1]="a" [2]="b" [3]="")
bar-foo
declare -a BASH_REMATCH=([0]="aaa" [1]="a")
declare -a BASH_REMATCH=([0]="abcd" [1]="a" [2]="bcd" [3]="")
0 1 <>
0
Wiggles 01
declare -a BASH_REMATCH=([0]="q")
declare -a BASH_REMATCH=()
declare -a BASH_REMATCH=([0]="b" [1]="b")
declare -a BASH_REMATCH=([0]="d" [1]="d")
status 0
status 0
done

== stderr 177
bash: line 15: declare: BASH_REMATCH: not found
bash: line 17: declare: BASH_REMATCH: cannot convert indexed to associative array
bash: line 18: BASH_REMATCH: readonly variable

== merged 576
0 b
1 0
declare -a BASH_REMATCH=()
0 4
declare -a BASH_REMATCH=([0]="ab" [1]="a" [2]="b" [3]="")
bar-foo
declare -a BASH_REMATCH=([0]="aaa" [1]="a")
declare -a BASH_REMATCH=([0]="abcd" [1]="a" [2]="bcd" [3]="")
0 1 <>
0
Wiggles 01
declare -a BASH_REMATCH=([0]="q")
declare -a BASH_REMATCH=()
declare -a BASH_REMATCH=([0]="b" [1]="b")
bash: line 15: declare: BASH_REMATCH: not found
declare -a BASH_REMATCH=([0]="d" [1]="d")
bash: line 17: declare: BASH_REMATCH: cannot convert indexed to associative array
status 0
bash: line 18: BASH_REMATCH: readonly variable
status 0
done

== status 0
