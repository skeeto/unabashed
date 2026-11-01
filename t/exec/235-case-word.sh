# expansions in the case word: no splitting or globbing
v='a  b'
case $v in 'a  b') echo "no split";; *) echo "split";; esac
s='*'
case $s in '*') echo "no glob";; esac
case ~ in /home/oracle) echo "tilde";; *) echo "no tilde: ~";; esac
case "" in '') echo "empty matches empty";; esac
case $unset in '') echo "unset is empty";; esac
case $(echo hi) in hi) echo "cmdsub";; esac
case $((2+3)) in 5) echo "arith";; esac
case "${v% *}" in a) echo "param op [${v% *}]";; *) echo "other [${v% *}]";; esac
case x$v in xa*b) echo "concat";; esac
case abc in *$(echo b)*) echo "cmdsub pattern";; esac
case a in ${unset}a) echo "empty var in pattern";; esac
case "" in ${unset}) echo "pattern only empty var";; esac
