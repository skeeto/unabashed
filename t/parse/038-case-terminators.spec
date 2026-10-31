== stdout 201
f () 
{ 
    case $1 in 
        a)
            echo a
        ;&
        b)
            echo b
        ;;&
        c)
            echo c
        ;;
        d)
            echo d
        ;;
    esac
}

== stderr 0

== merged 201
f () 
{ 
    case $1 in 
        a)
            echo a
        ;&
        b)
            echo b
        ;;&
        c)
            echo c
        ;;
        d)
            echo d
        ;;
    esac
}

== status 0
