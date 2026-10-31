== stdout 126
f () 
{ 
    while cat <<A |
x
A
  grep x;    do
        :;
    done;
    until a && cat <<B
b
B
    do
        :;
    done
}

== stderr 0

== merged 126
f () 
{ 
    while cat <<A |
x
A
  grep x;    do
        :;
    done;
    until a && cat <<B
b
B
    do
        :;
    done
}

== status 0
