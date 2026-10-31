== stdout 198
f () 
{ 
    cat <<E &
a
E
  cat <<E &
b
E
  cat <<F
c
F

    time cat <<E |
d
E
  cat
    ! cat <<E 2>&1 |
e
E
  cat
    time -p ! cat <<E
f
E

    cat <<E || 
g
E
 cat <<F && 
h
F
 cat <<G
i
G

}

== stderr 0

== merged 198
f () 
{ 
    cat <<E &
a
E
  cat <<E &
b
E
  cat <<F
c
F

    time cat <<E |
d
E
  cat
    ! cat <<E 2>&1 |
e
E
  cat
    time -p ! cat <<E
f
E

    cat <<E || 
g
E
 cat <<F && 
h
F
 cat <<G
i
G

}

== status 0
