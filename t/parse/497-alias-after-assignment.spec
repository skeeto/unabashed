== stdout 102
f () 
{ 
    a=1 echo X;
    a=1 b=2 echo X y;
    a=(1 2) echo X;
    a=1 x > out;
    a+=1 echo X
}

== stderr 0

== merged 102
f () 
{ 
    a=1 echo X;
    a=1 b=2 echo X y;
    a=(1 2) echo X;
    a=1 x > out;
    a+=1 echo X
}

== status 0
