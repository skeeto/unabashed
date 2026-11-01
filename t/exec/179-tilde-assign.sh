# Tilde expansion in assignments: after = and after each unquoted :.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
a=~
show "$a"
a=~/x:~/y:x~:~
show "$a"
a=x:~nosuch:~+
show "$a"
a="~":~
show "$a"
a=~"/q"
show "$a"
a=pre~
show "$a"
export b=~/e:~
show "$b"
declare c=~/d:~/f
show "$c"
f() { local l=~/l:~; show "$l"; }
f
show a=~ a=~/x:~/y x=y:~ --opt=~
show =~ :~ a:~
