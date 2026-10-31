== stdout 168
f () 
{ 
    case $1 in 
        a)
            echo a
        ;;
        b | c)
            echo bc
        ;;
        *)
            echo other
        ;;
    esac
}

== stderr 0

== merged 168
f () 
{ 
    case $1 in 
        a)
            echo a
        ;;
        b | c)
            echo bc
        ;;
        *)
            echo other
        ;;
    esac
}

== status 0
