== stdout 51
f () 
{ 
    exec {a[1]}> f {b[$i]}< g {c[k]}>&-
}

== stderr 0

== merged 51
f () 
{ 
    exec {a[1]}> f {b[$i]}< g {c[k]}>&-
}

== status 0
