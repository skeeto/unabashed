== stdout 106
f () 
{ 
    cat <<A |
alpha
A
  grep a
    echo next
}
g () 
{ 
    cat <<A |
one
A
  wc -l <<B
two
B

}

== stderr 0

== merged 106
f () 
{ 
    cat <<A |
alpha
A
  grep a
    echo next
}
g () 
{ 
    cat <<A |
one
A
  wc -l <<B
two
B

}

== status 0
