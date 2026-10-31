f() {
    a=([0]=x [1]= [2]+=y z=w $'\t' $"l" `bq` {a,b} ~ * [k]=$'v')
    declare -A m=( [a b]=1 ["c"]=2 [$(echo d)]=3 )
    b=(if then { } [[ ]] !)
}
declare -f
