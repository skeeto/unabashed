# $_ in nested synchronous subshells: a level's fresh buffer is sized by
# its value, not doubled from the level around it (which ran out of
# memory at about 27 levels)
f() {
    : "$1"
    if (($1 < 60)); then ( f $(($1+1)) ); else echo "depth $1, _=$_"; fi
}
f 1
echo "after: $_"
x=$(f 50; : last)
echo "$x"
