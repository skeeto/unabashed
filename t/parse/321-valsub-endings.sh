f() {
    a=${|x & }
    b=${| ( x ) }
    c=${|	REPLY=1
}
    d=${|while a; do b; done}
    e=${|(( REPLY = 2 )); }
}
declare -f
