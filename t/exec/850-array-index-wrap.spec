== stdout 488
declare -a b=([9223372036854775807]="p")
declare -a b=([9223372036854775806]="p" [9223372036854775807]="q")
declare -a b=([9223372036854775806]="p" [9223372036854775807]="q")
z 
declare -a f=([0]="1" [1]="2" [9223372036854775807]="zy")
declare -a f=([0]="1" [1]="2" [9223372036854775806]="w" [9223372036854775807]="zy")
declare -a f=([0]="1" [1]="2" [9223372036854775806]="w")
declare -a g=([1]="a" [9223372036854775807]="c")
st=1
st=0
declare -a m=([0]=$'2\n' [4294967295]=$'1\n')
<z><>

== stderr 175
bash: line 1: b[-9223372036854775808]: bad array subscript
bash: line 4: b[-9223372036854775808]: bad array subscript
bash: line 10: mapfile: 4294967296: invalid array origin

== merged 663
bash: line 1: b[-9223372036854775808]: bad array subscript
declare -a b=([9223372036854775807]="p")
declare -a b=([9223372036854775806]="p" [9223372036854775807]="q")
bash: line 4: b[-9223372036854775808]: bad array subscript
declare -a b=([9223372036854775806]="p" [9223372036854775807]="q")
z 
declare -a f=([0]="1" [1]="2" [9223372036854775807]="zy")
declare -a f=([0]="1" [1]="2" [9223372036854775806]="w" [9223372036854775807]="zy")
declare -a f=([0]="1" [1]="2" [9223372036854775806]="w")
declare -a g=([1]="a" [9223372036854775807]="c")
bash: line 10: mapfile: 4294967296: invalid array origin
st=1
st=0
declare -a m=([0]=$'2\n' [4294967295]=$'1\n')
<z><>

== status 0
