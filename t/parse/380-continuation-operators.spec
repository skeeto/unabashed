== stdout 159
f () 
{ 
    a && b || c;
    d >> e <<< f;
    case x in 
        y)
            g
        ;;
        z)
            h
        ;;&
    esac;
    i 2>&1 | j
}

== stderr 0

== merged 159
f () 
{ 
    a && b || c;
    d >> e <<< f;
    case x in 
        y)
            g
        ;;
        z)
            h
        ;;&
    esac;
    i 2>&1 | j
}

== status 0
