== stdout 94
f () 
{ 
    x=$(cat <<A && 
a
A
 echo y || cat <<B
b
B
);
    z=$(cat <<A &
a
A
  echo bg)
}

== stderr 0

== merged 94
f () 
{ 
    x=$(cat <<A && 
a
A
 echo y || cat <<B
b
B
);
    z=$(cat <<A &
a
A
  echo bg)
}

== status 0
