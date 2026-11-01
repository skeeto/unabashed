# eval with positional parameters and quoting of $@
set -- 'a b' c
eval 'for w in "$@"; do echo "<$w>"; done'
eval "echo $#"
eval 'set -- "$@" d'
echo "$# $3"
eval "set -- $*"
echo "$#"
f() { eval "echo \$$1"; }
f 2
q='"x  y"'
eval "set -- $q"
echo "[$1]"
