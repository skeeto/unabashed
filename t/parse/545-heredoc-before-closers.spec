== stdout 468
f () 
{ 
    if true; then
        cat <<E
x
E

    fi
    while false; do
        cat <<E
y
E

    done
    case x in 
        x)
            cat <<E
z
E

        ;;
    esac
    { 
        cat <<E
w
E

    };
    ( cat <<E
v
E
 );
    if true; then
        cat <<E
u
E

    fi
    { 
        cat <<E
t
E

    };
    ( cat <<E
s
E
 );
    case x in 
        x)
            cat <<E
r
E

        ;;
    esac
    for i in 1;
    do
        cat <<E
q
E

    done | cat
}

== stderr 0

== merged 468
f () 
{ 
    if true; then
        cat <<E
x
E

    fi
    while false; do
        cat <<E
y
E

    done
    case x in 
        x)
            cat <<E
z
E

        ;;
    esac
    { 
        cat <<E
w
E

    };
    ( cat <<E
v
E
 );
    if true; then
        cat <<E
u
E

    fi
    { 
        cat <<E
t
E

    };
    ( cat <<E
s
E
 );
    case x in 
        x)
            cat <<E
r
E

        ;;
    esac
    for i in 1;
    do
        cat <<E
q
E

    done | cat
}

== status 0
