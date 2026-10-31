== stdout 85
f () 
{ 
    echo '\U0001F600' '\u0080' '\U0010FFFF' 'A' 'A42' '\u' '\U' '\uZ' ''
}

== stderr 0

== merged 85
f () 
{ 
    echo '\U0001F600' '\u0080' '\U0010FFFF' 'A' 'A42' '\u' '\U' '\uZ' ''
}

== status 0
