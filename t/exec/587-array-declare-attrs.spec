== stdout 701
declare -a a
declare -a a=()
declare -a a=([0]="x")
declare -ax ex=([0]="1" [1]="2")
done
declare -al low=([0]="a" [1]="b")
declare -al low=([0]="a" [1]="b" [2]="c" [5]="dd")
declare -Au up=([x]="W" [k]="V" )
declare -a s=([0]="str")
declare -a b=([2]="x")
declare -a c
declare -a q=([0]="1" [1]="2" [2]="/home/oracle")
declare -a v=([0]="1" [1]="2")
declare -- n="(1 2)"
declare -A A
declare -A A=([x]="1" )
declare -A loc=([k]="v" )
declare -a la=([0]="1")
declare -a la=([0]="1")
declare -A loc=([k]="v" )
declare -a ga=([0]="1" [1]="2")
declare -a ty=([0]="1")
declare -a ty=([0]="1")
declare -a a=([0]="x") declare -ax ex=([0]="1" [1]="2")
declare -a a=([0]="x")
declare -ax ex=([0]="1" [1]="2")

== stderr 106
bash: line 2: r: readonly variable
bash: line 13: declare: ty: cannot destroy array variables in this way

== merged 807
declare -a a
declare -a a=()
declare -a a=([0]="x")
bash: line 2: r: readonly variable
declare -ax ex=([0]="1" [1]="2")
done
declare -al low=([0]="a" [1]="b")
declare -al low=([0]="a" [1]="b" [2]="c" [5]="dd")
declare -Au up=([x]="W" [k]="V" )
declare -a s=([0]="str")
declare -a b=([2]="x")
declare -a c
declare -a q=([0]="1" [1]="2" [2]="/home/oracle")
declare -a v=([0]="1" [1]="2")
declare -- n="(1 2)"
declare -A A
declare -A A=([x]="1" )
declare -A loc=([k]="v" )
declare -a la=([0]="1")
declare -a la=([0]="1")
declare -A loc=([k]="v" )
declare -a ga=([0]="1" [1]="2")
declare -a ty=([0]="1")
bash: line 13: declare: ty: cannot destroy array variables in this way
declare -a ty=([0]="1")
declare -a a=([0]="x") declare -ax ex=([0]="1" [1]="2")
declare -a a=([0]="x")
declare -ax ex=([0]="1" [1]="2")

== status 0
