== stdout 405
f () 
{ 
    case $x in 
        *.c | *.h)
            :
        ;;
        [abc]*)
            :
        ;;
        ?)
            :
        ;;
        'quoted*')
            :
        ;;
        "dq $y")
            :
        ;;
        \*)
            :
        ;;
        $var)
            :
        ;;
        "")
            :
        ;;
        in | esac | do)
            :
        ;;
    esac
}

== stderr 0

== merged 405
f () 
{ 
    case $x in 
        *.c | *.h)
            :
        ;;
        [abc]*)
            :
        ;;
        ?)
            :
        ;;
        'quoted*')
            :
        ;;
        "dq $y")
            :
        ;;
        \*)
            :
        ;;
        $var)
            :
        ;;
        "")
            :
        ;;
        in | esac | do)
            :
        ;;
    esac
}

== status 0
