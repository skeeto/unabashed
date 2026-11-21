== stdout 189
zy
s=0
s=1
e
zy
s=0
alias e=''
alias zy='2'
alias e=''
alias zy='2'
zy e n=2
declare -A BASH_ALIASES=([zy]="2" [e]="" )
1
1
zy
alias e=''
alias zy='2'
sub 2
e
zy
zy
zz zy a2 a3 a1 a4 e
s=1

== stderr 35
bash: line 11: type: zz: not found

== merged 224
zy
s=0
s=1
e
zy
s=0
alias e=''
alias zy='2'
alias e=''
alias zy='2'
zy e n=2
declare -A BASH_ALIASES=([zy]="2" [e]="" )
bash: line 11: type: zz: not found
1
1
zy
alias e=''
alias zy='2'
sub 2
e
zy
zy
zz zy a2 a3 a1 a4 e
s=1

== status 0
