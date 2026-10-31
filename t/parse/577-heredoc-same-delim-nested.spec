== stdout 170
f () 
{ 
    cat $(cat <<E
inner
E
) <<E
outer
E

    echo $(cat <<-E
x
E
)
    echo $(cat $(cat $(cat <<C
c
C
) <<B
b
B
) <<A
a
A
);
    x=$({ cat; } <<E |
y
E
  cat)
}

== stderr 0

== merged 170
f () 
{ 
    cat $(cat <<E
inner
E
) <<E
outer
E

    echo $(cat <<-E
x
E
)
    echo $(cat $(cat $(cat <<C
c
C
) <<B
b
B
) <<A
a
A
);
    x=$({ cat; } <<E |
y
E
  cat)
}

== status 0
