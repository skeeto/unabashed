== stdout 1189
declare -i i="7"
declare -l l="abc"
declare -u u="ABC"
declare -c c="Hello"
declare -t t="1"
declare -x x="1"
declare -ixl ilx="5"
declare -- lu="Mixed"
declare -- ul="Mixed"
declare -- cu="mixed"
declare -i i="9"
declare -- i="a+b"
declare -- i="a+b"
declare -a BASH_ARGC=()
declare -a BASH_ARGV=()
declare -a BASH_LINENO=()
declare -a BASH_SOURCE=()
declare -ar BASH_VERSINFO=([0]="5" [1]="3" [2]="0" [3]="1" [4]="release" [5]="x86_64-pc-linux-gnu")
declare -a DIRSTACK=()
declare -a FUNCNAME
declare -a GROUPS=()
declare -a PIPESTATUS=([0]="0")
declare -A BASH_ALIASES=()
declare -A BASH_CMDS=()
declare -ixl ilx="5"
declare -ixl ilx="5"
declare -x x="1"
declare -t t="1"
declare -- x="1"
declare -- i="a+b"
declare -l l="abc"
declare -- gg="2"
declare -- lv="1"
declare -- lv="1"
declare -- gg="2"
bash: line 17: local: can only be used in a function
1
bash: line 18: declare: -z: invalid option
declare: usage: declare [-aAfFgiIlnrtux] [name[=value] ...] or declare -p [-aAfFilnrtux] [name ...]
2
bash: line 19: declare: nosuch: not found
1
f () 
{ 
local -
hBc
declare -- a="1"
declare -- b
declare -- a="1"
declare -- a="1"
declare -- b
1
declare -r ro="1"
declare -n nr="ro"
r r r

== stderr 46
bash: line 23: declare: ro: readonly variable

== merged 1235
declare -i i="7"
declare -l l="abc"
declare -u u="ABC"
declare -c c="Hello"
declare -t t="1"
declare -x x="1"
declare -ixl ilx="5"
declare -- lu="Mixed"
declare -- ul="Mixed"
declare -- cu="mixed"
declare -i i="9"
declare -- i="a+b"
declare -- i="a+b"
declare -a BASH_ARGC=()
declare -a BASH_ARGV=()
declare -a BASH_LINENO=()
declare -a BASH_SOURCE=()
declare -ar BASH_VERSINFO=([0]="5" [1]="3" [2]="0" [3]="1" [4]="release" [5]="x86_64-pc-linux-gnu")
declare -a DIRSTACK=()
declare -a FUNCNAME
declare -a GROUPS=()
declare -a PIPESTATUS=([0]="0")
declare -A BASH_ALIASES=()
declare -A BASH_CMDS=()
declare -ixl ilx="5"
declare -ixl ilx="5"
declare -x x="1"
declare -t t="1"
declare -- x="1"
declare -- i="a+b"
declare -l l="abc"
declare -- gg="2"
declare -- lv="1"
declare -- lv="1"
declare -- gg="2"
bash: line 17: local: can only be used in a function
1
bash: line 18: declare: -z: invalid option
declare: usage: declare [-aAfFgiIlnrtux] [name[=value] ...] or declare -p [-aAfFilnrtux] [name ...]
2
bash: line 19: declare: nosuch: not found
1
f () 
{ 
local -
hBc
declare -- a="1"
declare -- b
declare -- a="1"
declare -- a="1"
declare -- b
bash: line 23: declare: ro: readonly variable
1
declare -r ro="1"
declare -n nr="ro"
r r r

== status 0
