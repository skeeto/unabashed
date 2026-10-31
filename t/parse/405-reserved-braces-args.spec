== stdout 230
f () 
{ 
    { 
        echo }
    };
    { 
        echo a}
    };
    case x in 
        (esac)
            a
        ;;
    esac;
    case x in 
        a | esac)
            b
        ;;
    esac;
    echo ]] [[ ! { } in do
}

== stderr 0

== merged 230
f () 
{ 
    { 
        echo }
    };
    { 
        echo a}
    };
    case x in 
        (esac)
            a
        ;;
    esac;
    case x in 
        a | esac)
            b
        ;;
    esac;
    echo ]] [[ ! { } in do
}

== status 0
