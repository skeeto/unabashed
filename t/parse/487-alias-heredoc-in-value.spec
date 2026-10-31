== stdout 139
f () 
{ 
    cat <<END
inside
END

    echo after
}
g () 
{ 
    cat <<E
first rest
E

    echo next
}
k () 
{ 
    echo $(cat <<C
x
C
)
}

== stderr 0

== merged 139
f () 
{ 
    cat <<END
inside
END

    echo after
}
g () 
{ 
    cat <<E
first rest
E

    echo next
}
k () 
{ 
    echo $(cat <<C
x
C
)
}

== status 0
