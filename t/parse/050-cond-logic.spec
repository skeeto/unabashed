== stdout 274
f () 
{ 
    [[ -n a && -n b ]];
    [[ -n a || -n b ]];
    [[ ! -n a ]];
    [[ -n a ]];
    [[ -n a && -n b || -n c && -n d ]];
    [[ -n a || -n b && -n c ]];
    [[ ( -n a || -n b ) && -n c ]];
    [[ ! ( -n a && -n b ) ]];
    [[ ( ( -n a ) ) ]];
    [[ ( -n x ) ]]
}

== stderr 0

== merged 274
f () 
{ 
    [[ -n a && -n b ]];
    [[ -n a || -n b ]];
    [[ ! -n a ]];
    [[ -n a ]];
    [[ -n a && -n b || -n c && -n d ]];
    [[ -n a || -n b && -n c ]];
    [[ ( -n a || -n b ) && -n c ]];
    [[ ! ( -n a && -n b ) ]];
    [[ ( ( -n a ) ) ]];
    [[ ( -n x ) ]]
}

== status 0
