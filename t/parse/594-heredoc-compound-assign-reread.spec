== stdout 56
f () 
{ 
    cat <<'' <<



    a=(1 2)
    echo next
}

== stderr 0

== merged 56
f () 
{ 
    cat <<'' <<



    a=(1 2)
    echo next
}

== status 0
