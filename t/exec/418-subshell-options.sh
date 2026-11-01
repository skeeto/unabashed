# Options and shopts set in a subshell do not leak
(set -u; echo "in: $-" | { read x; case $x in *u*) echo has-u;; esac; })
case $- in *u*) echo leak;; *) echo no-leak;; esac
(set -f; echo /*)
echo /tm*
(shopt -s nullglob; set -- /nomatch*; echo "count $#")
set -- /nomatch*; echo "count $# $1"
(set -o noglob; shopt -o noglob)
shopt -o noglob
(IFS=:; x="a:b"; set -- $x; echo $#)
x="a:b"; set -- $x; echo $#
