== stdout 159
f () 
{ 
    while a; do
        until b; do
            while c; do
                break 2;
            done;
            continue;
        done;
    done
}

== stderr 0

== merged 159
f () 
{ 
    while a; do
        until b; do
            while c; do
                break 2;
            done;
            continue;
        done;
    done
}

== status 0
