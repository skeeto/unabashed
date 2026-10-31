== stdout 178
f () 
{ 
    select x in a b c;
    do
        echo $x;
        break;
    done;
    select x in "$@";
    do
        :;
    done;
    select x in ;
    do
        :;
    done
}

== stderr 0

== merged 178
f () 
{ 
    select x in a b c;
    do
        echo $x;
        break;
    done;
    select x in "$@";
    do
        :;
    done;
    select x in ;
    do
        :;
    done
}

== status 0
