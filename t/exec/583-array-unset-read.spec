== stdout 506
declare -a a=([0]="1" [2]="3" [3]="4")
declare -a a=([0]="1" [2]="3")
st=1
declare -A h=([y]="2" )
declare -A h=([y]="2" )
declare -a a=()
declare -a a=()
declare -- s="str"
declare -a r=([0]="x" [1]="y" [2]="z")
declare -a r2=([0]="a\\" [1]="b" [2]="c")
declare -a r3=([0]="1" [1]="" [2]="2")
declare -a a=([0]="xz" [1]="y")
declare -a a=([0]="w" [1]="y")
declare -a q=([0]="1")
declare -a q=([3]="a")
declare -a q=([0]="z")
declare -A ass=([c]="" [a]="b" )
declare -A h2=([a]="" )
declare -a a=([0]="1")

== stderr 305
bash: line 1: unset: [-9]: bad array subscript
bash: line 4: declare: s: not found
bash: line 4: unset: s: not an array variable
bash: line 9: ass: x: must use subscript when assigning associative array
bash: line 10: ['']='x': bad array subscript
bash: line 12: [@]=3: cannot assign to non-numeric index

== merged 811
declare -a a=([0]="1" [2]="3" [3]="4")
declare -a a=([0]="1" [2]="3")
bash: line 1: unset: [-9]: bad array subscript
st=1
declare -A h=([y]="2" )
declare -A h=([y]="2" )
declare -a a=()
declare -a a=()
bash: line 4: declare: s: not found
bash: line 4: unset: s: not an array variable
declare -- s="str"
declare -a r=([0]="x" [1]="y" [2]="z")
declare -a r2=([0]="a\\" [1]="b" [2]="c")
declare -a r3=([0]="1" [1]="" [2]="2")
declare -a a=([0]="xz" [1]="y")
declare -a a=([0]="w" [1]="y")
declare -a q=([0]="1")
declare -a q=([3]="a")
declare -a q=([0]="z")
declare -A ass=([c]="" [a]="b" )
bash: line 9: ass: x: must use subscript when assigning associative array
declare -A h2=([a]="" )
bash: line 10: ['']='x': bad array subscript
declare -a a=([0]="1")
bash: line 12: [@]=3: cannot assign to non-numeric index

== status 1
