== stdout 134
f () 
{ 
    cat <<A |
a $(cat <<D
d1 $(cat <<E
e1
E
)
D
) a2
A
  tr a b <<B |
b1 `cat <<F
f1
F
`
B
  cat - <<C
c1
C

    echo done
}

== stderr 0

== merged 134
f () 
{ 
    cat <<A |
a $(cat <<D
d1 $(cat <<E
e1
E
)
D
) a2
A
  tr a b <<B |
b1 `cat <<F
f1
F
`
B
  cat - <<C
c1
C

    echo done
}

== status 0
