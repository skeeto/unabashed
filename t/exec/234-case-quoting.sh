# quoted parts of case patterns match literally
p='*'
for w in '*' x; do
    case $w in "*") echo "$w: dq star";; *) echo "$w: other";; esac
    case $w in '*') echo "$w: sq star";; *) echo "$w: other";; esac
    case $w in \*) echo "$w: bs star";; *) echo "$w: other";; esac
    case $w in $p) echo "$w: unquoted var";; esac
    case $w in "$p") echo "$w: quoted var";; *) echo "$w: no quoted var";; esac
done
q='a?c'
case abc in $q) echo "var glob";; esac
case abc in "$q") echo no;; *) echo "quoted var literal";; esac
case 'a?c' in "$q") echo "literal match";; esac
case 'a|b' in a\|b) echo "escaped bar";; esac
case 'a|b' in 'a|b') echo "quoted bar";; esac
case b in a|b) echo "bar alternation";; esac
case ')' in ')') echo "paren";; esac
case 'x y' in x\ y) echo "escaped space";; esac
case 'x y' in 'x '*) echo "partial quote";; esac
