== stdout 124
f () 
{ 
    case x in 
        a)

        ;;
    esac;
    case y in 
        b)
            echo b
        ;;
    esac
}

== stderr 0

== merged 124
f () 
{ 
    case x in 
        a)

        ;;
    esac;
    case y in 
        b)
            echo b
        ;;
    esac
}

== status 0
