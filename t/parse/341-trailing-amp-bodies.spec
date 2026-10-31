== stdout 300
f () 
{ 
    if a & then
        b & c &
    fi;
    while a & do
        b;
    done;
    for i in 1;
    do
        a &
    done;
    until a; do
        { 
            b &
        };
    done;
    case x in 
        x)
            a & b &
        ;;
    esac;
    ( a & ) & echo a\&;
    echo b
}

== stderr 0

== merged 300
f () 
{ 
    if a & then
        b & c &
    fi;
    while a & do
        b;
    done;
    for i in 1;
    do
        a &
    done;
    until a; do
        { 
            b &
        };
    done;
    case x in 
        x)
            a & b &
        ;;
    esac;
    ( a & ) & echo a\&;
    echo b
}

== status 0
