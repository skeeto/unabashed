== stdout 395
declare -a FUNCNAME
declare -a BASH_SOURCE=()
declare -a BASH_LINENO=()
declare -a FUNCNAME=([0]="f" [1]="g")
declare -a BASH_SOURCE=([0]="bash" [1]="bash")
declare -a BASH_LINENO=([0]="4" [1]="6")
f g f 2
unset 8 u 1 0 h
k 9 9 2 0 1 k
5 3 0 1 release HOST 6
declare -ar BASH_VERSINFO=([0]="5" [1]="3" [2]="0" [3]="1" [4]="release" [5]="HOST")
declare -a GROUPS=([0]="1000")
declare -a FUNCNAME

== stderr 48
bash: line 11: BASH_VERSINFO: readonly variable

== merged 443
declare -a FUNCNAME
declare -a BASH_SOURCE=()
declare -a BASH_LINENO=()
declare -a FUNCNAME=([0]="f" [1]="g")
declare -a BASH_SOURCE=([0]="bash" [1]="bash")
declare -a BASH_LINENO=([0]="4" [1]="6")
f g f 2
unset 8 u 1 0 h
k 9 9 2 0 1 k
5 3 0 1 release HOST 6
declare -ar BASH_VERSINFO=([0]="5" [1]="3" [2]="0" [3]="1" [4]="release" [5]="HOST")
declare -a GROUPS=([0]="1000")
bash: line 11: BASH_VERSINFO: readonly variable
declare -a FUNCNAME

== status 0
