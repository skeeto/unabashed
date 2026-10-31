== stdout 63
f () 
{ 
    echo "${x:-	}" "${x:=a
b}" "${x+q'r}" "${x:?A}"
}

== stderr 0

== merged 63
f () 
{ 
    echo "${x:-	}" "${x:=a
b}" "${x+q'r}" "${x:?A}"
}

== status 0
