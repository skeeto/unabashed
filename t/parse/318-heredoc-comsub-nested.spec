== stdout 100
f () 
{ 
    x=$(echo $(cat <<A
inner
A
) outer);
    y=$(if cat <<B
b
B
    then
    echo t;
fi)
}

== stderr 0

== merged 100
f () 
{ 
    x=$(echo $(cat <<A
inner
A
) outer);
    y=$(if cat <<B
b
B
    then
    echo t;
fi)
}

== status 0
