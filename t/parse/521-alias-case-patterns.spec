== stdout 162
f () 
{ 
    case x in 
        x)
            echo X
        ;;
        x)
            echo X
        ;;
        x | x)
            echo X
        ;;
    esac
}

== stderr 0

== merged 162
f () 
{ 
    case x in 
        x)
            echo X
        ;;
        x)
            echo X
        ;;
        x | x)
            echo X
        ;;
    esac
}

== status 0
