echo "top: [${FUNCNAME-unset}]"
inner() { echo "inner: $FUNCNAME"; }
outer() { echo "outer: $FUNCNAME"; inner; echo "back in: $FUNCNAME"; }
outer
echo "top again: [${FUNCNAME-unset}]"
function kw { echo "kw: $FUNCNAME"; }
kw
r() { echo "r$1: $FUNCNAME"; [ $1 -lt 2 ] && r $(($1 + 1)); return 0; }
r 0
s() { x=$(echo "sub: $FUNCNAME"); echo "$x"; }
s
