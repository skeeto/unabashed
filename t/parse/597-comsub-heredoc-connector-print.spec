== stdout 208
f () 
{ 
    x=$(cat <<E |
x
E
  b c);
    x=$(cat <<E
x
E
 cat <<F
y
F
);
    x=$(while a; do
    cat <<E
x
E

done b);
    x=$({ cat <<E
x
E
 } | cat);
    x=$(function g () 
{ 
    a;
    b

    c
}; h)
}

== stderr 0

== merged 208
f () 
{ 
    x=$(cat <<E |
x
E
  b c);
    x=$(cat <<E
x
E
 cat <<F
y
F
);
    x=$(while a; do
    cat <<E
x
E

done b);
    x=$({ cat <<E
x
E
 } | cat);
    x=$(function g () 
{ 
    a;
    b

    c
}; h)
}

== status 0
