== stdout 266
f () 
{ 
    while a; do
        if b; then
            :;
        else
            if cat <<A
a
A
    then
                c;
            else
                if cat <<B
b
B
    then
                    d;
                fi;
            fi;
        fi;
    done
}

== stderr 0

== merged 266
f () 
{ 
    while a; do
        if b; then
            :;
        else
            if cat <<A
a
A
    then
                c;
            else
                if cat <<B
b
B
    then
                    d;
                fi;
            fi;
        fi;
    done
}

== status 0
