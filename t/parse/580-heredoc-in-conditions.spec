== stdout 339
f () 
{ 
    if cat <<E
x
E
    then
        :;
    fi;
    while cat <<E
y
E
    do
        :;
    done;
    until cat <<E
z
E

    false;    do
        :;
    done;
    case x in 
        x)

        ;;
    esac <<E
w
E

    select s in a;
    do
        break;
    done <<E
v
E

    [[ -n $(cat <<E
u
E
) ]]
    (( $(cat <<E
1
E
) ))
}

== stderr 0

== merged 339
f () 
{ 
    if cat <<E
x
E
    then
        :;
    fi;
    while cat <<E
y
E
    do
        :;
    done;
    until cat <<E
z
E

    false;    do
        :;
    done;
    case x in 
        x)

        ;;
    esac <<E
w
E

    select s in a;
    do
        break;
    done <<E
v
E

    [[ -n $(cat <<E
u
E
) ]]
    (( $(cat <<E
1
E
) ))
}

== status 0
