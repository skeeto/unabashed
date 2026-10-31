== stdout 46
f () 
{ 
    a & b & c & d && e & g | h & i
}

== stderr 0

== merged 46
f () 
{ 
    a & b & c & d && e & g | h & i
}

== status 0
