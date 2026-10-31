== stdout 69
f () 
{ 
    echo "${x:-"${y:-	}"}" ${x:-"$'\t'"} ${x:-"${y#'a'}"}
}

== stderr 0

== merged 69
f () 
{ 
    echo "${x:-"${y:-	}"}" ${x:-"$'\t'"} ${x:-"${y#'a'}"}
}

== status 0
