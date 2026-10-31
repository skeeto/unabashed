f() {
    { a; b; }
    ( a; b )
    { a; } && ( b )
    { a
      b; }
    (a)
}
declare -f
