== stdout 169
f () 
{ 
    select x in "$@";
    do
        echo $x;
    done;
    select x in a b;
    do
        break;
    done;
    select x in "$@";
    do
        :;
    done
}

== stderr 0

== merged 169
f () 
{ 
    select x in "$@";
    do
        echo $x;
    done;
    select x in a b;
    do
        break;
    done;
    select x in "$@";
    do
        :;
    done
}

== status 0
