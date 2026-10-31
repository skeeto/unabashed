== stdout 237
f () 
{ 
    x=$(if a; then
    b;
else
    c;
fi);
    y=$(for i in 1 2;
do
    echo $i;
done);
    z=$(while read; do
    :;
done < in);
    w=$({ a; b; } | c);
    v=$( ( sub ));
    u=$([[ -n a ]] && echo y);
    t=$( (( 1 + 2 )))
}

== stderr 0

== merged 237
f () 
{ 
    x=$(if a; then
    b;
else
    c;
fi);
    y=$(for i in 1 2;
do
    echo $i;
done);
    z=$(while read; do
    :;
done < in);
    w=$({ a; b; } | c);
    v=$( ( sub ));
    u=$([[ -n a ]] && echo y);
    t=$( (( 1 + 2 )))
}

== status 0
