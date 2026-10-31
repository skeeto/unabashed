== stdout 139
f () 
{ 
    case $1 in 
        a)
            echo a
        ;;
        b)
            echo b;
            echo bb
        ;;
    esac
}

== stderr 0

== merged 139
f () 
{ 
    case $1 in 
        a)
            echo a
        ;;
        b)
            echo b;
            echo bb
        ;;
    esac
}

== status 0
