== stdout 72
f () 
{ 
    echo "${x:-"abc"}" "${x#"a b"}" "${x/"p"/"r"}" ${x:-"u"}
}

== stderr 0

== merged 72
f () 
{ 
    echo "${x:-"abc"}" "${x#"a b"}" "${x/"p"/"r"}" ${x:-"u"}
}

== status 0
