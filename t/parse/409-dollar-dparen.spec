== stdout 114
f () 
{ 
    echo $( ( a )) $((1+2)) $( ( b ); c) $((c) ) "$( ( d ))";
    echo $((echo  x);(y)) "$((e)  |  f)"
}

== stderr 0

== merged 114
f () 
{ 
    echo $( ( a )) $((1+2)) $( ( b ); c) $((c) ) "$( ( d ))";
    echo $((echo  x);(y)) "$((e)  |  f)"
}

== status 0
