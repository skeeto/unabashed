== stdout 97
f () 
{ 
    echo 'tab	here' 'nl
' 'quote'\''s' 'AAé' '[0m' '';
    echo "translated $x" ''
}

== stderr 0

== merged 97
f () 
{ 
    echo 'tab	here' 'nl
' 'quote'\''s' 'AAé' '[0m' '';
    echo "translated $x" ''
}

== status 0
