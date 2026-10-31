== stdout 241
f () 
{ 
    for x in a b;
    do
        echo;
    done;
    case x in 
        x)

        ;;
    esac;
    for x in "$@";
    do
        :;
    done;
    for x in "$@";
    do
        :;
    done;
    echo $(for y in c;
do
    :;
done)
}

== stderr 0

== merged 241
f () 
{ 
    for x in a b;
    do
        echo;
    done;
    case x in 
        x)

        ;;
    esac;
    for x in "$@";
    do
        :;
    done;
    for x in "$@";
    do
        :;
    done;
    echo $(for y in c;
do
    :;
done)
}

== status 0
