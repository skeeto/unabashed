f() {
    local x=local-x
    echo "sub: $(echo "$x $1 $FUNCNAME")"
    y=$(x=changed; echo "$x")
    echo "y=$y x=$x"
    ( x=subshell; echo "in sub: $x" )
    echo "after sub: $x"
    z=$(local x=inner; echo "$x"); echo "z=$z x=$x"
}
f arg1
echo "x=[${x-unset}]"
g() ( local q=1; echo "q=$q" )
g
echo "q=[${q-unset}]"
