== stdout 149
f () 
{ 
    [[ $x == *.txt ]];
    [[ $x == "*.txt" ]];
    [[ $x != a?c ]];
    [[ $x == [!a-z]* ]];
    [[ $x = $pat ]];
    [[ $x == "$pat" ]]
}

== stderr 0

== merged 149
f () 
{ 
    [[ $x == *.txt ]];
    [[ $x == "*.txt" ]];
    [[ $x != a?c ]];
    [[ $x == [!a-z]* ]];
    [[ $x = $pat ]];
    [[ $x == "$pat" ]]
}

== status 0
