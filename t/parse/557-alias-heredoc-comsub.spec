== stdout 112
f () 
{ 
    cat <<E
body $x
E

    echo $(cat <<F
fbody
F
)
    cat | cat <<E
pipe
E

    echo $(hd
inner
E)
}

== stderr 0

== merged 112
f () 
{ 
    cat <<E
body $x
E

    echo $(cat <<F
fbody
F
)
    cat | cat <<E
pipe
E

    echo $(hd
inner
E)
}

== status 0
