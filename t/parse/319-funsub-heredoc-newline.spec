== stdout 72
f () 
{ 
    y=${ cat <<A
a
A

 };
    z=${ cat <<B
b
B

echo more
 }
}

== stderr 0

== merged 72
f () 
{ 
    y=${ cat <<A
a
A

 };
    z=${ cat <<B
b
B

echo more
 }
}

== status 0
