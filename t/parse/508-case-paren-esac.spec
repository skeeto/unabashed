== stdout 80
f () 
{ 
    case x in 
        (esac)
            echo y
        ;;
    esac
}

== stderr 0

== merged 80
f () 
{ 
    case x in 
        (esac)
            echo y
        ;;
    esac
}

== status 0
