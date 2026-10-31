== stdout 203
f () 
{ 
    case $1 in 
        a)
            echo one;
            echo two
        ;;
        b)
            if x; then
                y;
            fi
        ;&
        *)

        ;;
    esac
}

== stderr 0

== merged 203
f () 
{ 
    case $1 in 
        a)
            echo one;
            echo two
        ;;
        b)
            if x; then
                y;
            fi
        ;&
        *)

        ;;
    esac
}

== status 0
