== stdout 381
zero
hit 1
hit 1000
hit 2000
none
hit 2000
hit 7
hit 7
status 0
hit 3
hit 1999
f () 
{ 
    if [ "$1" = a ]; then
        echo A;
    else
        if [ "$1" = b ]; then
            echo B;
        else
            if [ "$1" = c ]; then
                echo C;
            else
                echo other;
            fi;
        fi;
    fi
}
C
other
k1
k2000
default
cond 0
cond 1

== stderr 0

== merged 381
zero
hit 1
hit 1000
hit 2000
none
hit 2000
hit 7
hit 7
status 0
hit 3
hit 1999
f () 
{ 
    if [ "$1" = a ]; then
        echo A;
    else
        if [ "$1" = b ]; then
            echo B;
        else
            if [ "$1" = c ]; then
                echo C;
            else
                echo other;
            fi;
        fi;
    fi
}
C
other
k1
k2000
default
cond 0
cond 1

== status 0
