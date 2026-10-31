== stdout 146
f () 
{ 
    case $x in 
        a)
            echo a
        ;;
        b | c)
            echo bc
        ;;
        d)

        ;;
    esac
}

== stderr 0

== merged 146
f () 
{ 
    case $x in 
        a)
            echo a
        ;;
        b | c)
            echo bc
        ;;
        d)

        ;;
    esac
}

== status 0
