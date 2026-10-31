== stdout 214
f () 
{ 
    { 
        a
    } | ( b ) | while read; do
        :;
    done | if x; then
        y;
    fi;
    for i in 1;
    do
        :;
    done && case x in 
        *)

        ;;
    esac || [[ -n y ]]
}

== stderr 0

== merged 214
f () 
{ 
    { 
        a
    } | ( b ) | while read; do
        :;
    done | if x; then
        y;
    fi;
    for i in 1;
    do
        :;
    done && case x in 
        *)

        ;;
    esac || [[ -n y ]]
}

== status 0
