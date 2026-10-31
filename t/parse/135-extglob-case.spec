== stdout 182
f () 
{ 
    case $x in 
        @(a|b))

        ;;
        !(c))

        ;;
        +([0-9]))

        ;;
        *(x)?(y))

        ;;
        @(*.c|*.h))

        ;;
    esac
}

== stderr 0

== merged 182
f () 
{ 
    case $x in 
        @(a|b))

        ;;
        !(c))

        ;;
        +([0-9]))

        ;;
        *(x)?(y))

        ;;
        @(*.c|*.h))

        ;;
    esac
}

== status 0
