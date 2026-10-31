== stdout 183
f () 
{ 
    coproc cat <<E
x
E

    coproc NAME { 
        cat <<E
y
E

    };
    coproc COPROC ( cat <<E
z
E
 );
    coproc W while read l; do
        echo $l;
    done <<E
w
E

}

== stderr 0

== merged 183
f () 
{ 
    coproc cat <<E
x
E

    coproc NAME { 
        cat <<E
y
E

    };
    coproc COPROC ( cat <<E
z
E
 );
    coproc W while read l; do
        echo $l;
    done <<E
w
E

}

== status 0
