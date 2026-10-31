== stdout 181
f () 
{ 
    diff <(a) <(b);
    tee >(c) >(d | e) > /dev/null;
    cat < <(cmd);
    a > >(b);
    echo <({ x; y; }) >( ( z ));
    while read l; do
        :;
    done < <(gen)
}

== stderr 0

== merged 181
f () 
{ 
    diff <(a) <(b);
    tee >(c) >(d | e) > /dev/null;
    cat < <(cmd);
    a > >(b);
    echo <({ x; y; }) >( ( z ));
    while read l; do
        :;
    done < <(gen)
}

== status 0
