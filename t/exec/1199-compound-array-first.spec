== stdout 395
declare -a a=([0]="declare -a a=()")
declare -a x=([0]="declare -a x=([0]=\"s\")")
declare -a x=([0]="s" [1]="declare -a x=([0]=\"s\")")
declare -a t=([0]="declare -a t=()")
declare -a l=([0]="declare -a l")
declare -a b=([0]="1")
declare -a arr=()
declare -a y=([0]="1")
declare -a z=([0]="")
declare -a w=()
declare -ai n=([0]="5")
declare -A m=([k]="v" )
bash: line 23: declare: v: not found

== stderr 335
bash: line 10: 1/0: division by 0 (error token is "0")
bash: line 12: u: invalid indirect expansion
bash: line 14: 1/0: division by 0 (error token is "0")
bash: line 16: u: invalid indirect expansion
bash: line 18: u: invalid indirect expansion
bash: line 20: u: invalid indirect expansion
bash: line 22: u: invalid indirect expansion

== merged 730
declare -a a=([0]="declare -a a=()")
declare -a x=([0]="declare -a x=([0]=\"s\")")
declare -a x=([0]="s" [1]="declare -a x=([0]=\"s\")")
declare -a t=([0]="declare -a t=()")
declare -a l=([0]="declare -a l")
declare -a b=([0]="1")
bash: line 10: 1/0: division by 0 (error token is "0")
declare -a arr=()
bash: line 12: u: invalid indirect expansion
declare -a y=([0]="1")
bash: line 14: 1/0: division by 0 (error token is "0")
declare -a z=([0]="")
bash: line 16: u: invalid indirect expansion
declare -a w=()
bash: line 18: u: invalid indirect expansion
declare -ai n=([0]="5")
bash: line 20: u: invalid indirect expansion
declare -A m=([k]="v" )
bash: line 22: u: invalid indirect expansion
bash: line 23: declare: v: not found

== status 1
