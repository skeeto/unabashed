# Arguments that look like assignments get tilde expansion after the
# first = and after each unquoted : (outside posix mode).
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show a=~ b=~/x c=x:~:~/y
show a=\~ a='~' a=~x a=x~ a==~
show 1a=~ -a=~ a.b=~ "a"=~
show a+=~ _=~ A1=~
show a=b=~ a=b:~
x=~
show v=$x
show a=~:"$x"
set -o posix
show a=~ c=x:~
set +o posix
show a=~
