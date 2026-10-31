== stdout 41
g () 
{ 
    case "$(! )" in 
    esac
}

== stderr 0

== merged 41
g () 
{ 
    case "$(! )" in 
    esac
}

== status 0
