# [[ == ]] and != use extglob patterns even with extglob off, and
# nocasematch folds case inside groups.
shopt -u extglob
t() { if [[ $1 == $2 ]]; then echo "$1 $2 yes"; else echo "$1 $2 no"; fi; }
t abc '+([a-c])'
t abcd '+([a-c])'
t foo.c '*.@(c|h)'
t foo.o '*.@(c|h)'
t foo '!(*.*)'
t foo.x '!(*.*)'
t aaab '*(a)b'
t '' '*(a)'
t '' '+(a)'
t ab '@(a|ab)'
t ab '?(a)?(b)'
[[ abc != @(abc) ]] && echo ne1 || echo ne2
shopt -s nocasematch
t ABC '+(a|b|c)'
t Foo.C '*.@(c|h)'
t XYZ '!(xyz)'
shopt -u nocasematch
p='@(x|y)'
[[ x == $p ]] && echo unquoted var
[[ x == "$p" ]] && echo quoted var || echo quoted var literal
[[ '@(x|y)' == "$p" ]] && echo literal match
