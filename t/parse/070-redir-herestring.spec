== stdout 99
f () 
{ 
    a <<< word;
    a <<< "two words";
    a <<< $x;
    a 3<<< 'q';
    a <<< "$(cmd)"
}

== stderr 0

== merged 99
f () 
{ 
    a <<< word;
    a <<< "two words";
    a <<< $x;
    a 3<<< 'q';
    a <<< "$(cmd)"
}

== status 0
