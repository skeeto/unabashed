== stdout 121
20000 20000
28893
3
declare -a a=([0]="1" [1]="5" [2]="1")
2
declare -a b=([3]="-1")
3+4
8
declare -i w="2*3"
next
after

== stderr 166
bash: command substitution: line 14: unexpected EOF while looking for matching `)'
bash: command substitution: line 15: unexpected EOF while looking for matching `''

== merged 287
20000 20000
28893
3
declare -a a=([0]="1" [1]="5" [2]="1")
2
declare -a b=([3]="-1")
3+4
8
declare -i w="2*3"
bash: command substitution: line 14: unexpected EOF while looking for matching `)'
next
bash: command substitution: line 15: unexpected EOF while looking for matching `''
after

== status 0
