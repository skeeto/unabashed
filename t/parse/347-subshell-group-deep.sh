f() {
    ( { ( a; b ) } )
    { ( a ); { b; }; }
    ( a; { b; c; }; d )
    ( a | { b; c; } )
    { a; ( b; c ); } | d
}
declare -f
