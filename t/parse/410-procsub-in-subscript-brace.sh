f() {
    a[<(  x  )]=1
    echo ${y:-<(  z  )} "${y:-<(  w  )}" ${y:->(  v  )} ${y:-<  (u)}
}
declare -f
