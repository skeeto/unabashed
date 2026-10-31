== stdout 110
f () 
{ 
    cat <<''
a $x


    echo one
    cat <<''
b $y


    echo two
    cat <<-''
c


    echo three
}

== stderr 0

== merged 110
f () 
{ 
    cat <<''
a $x


    echo one
    cat <<''
b $y


    echo two
    cat <<-''
c


    echo three
}

== status 0
