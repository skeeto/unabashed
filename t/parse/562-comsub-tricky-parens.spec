== stdout 287
f () 
{ 
    echo $(echo \) ')' ")" ')' ${x#)} $((1)) "${y%%)*}");
    echo $(echo $(( (1+2) )) $( ( echo a )) $((echo b) ));
    echo $((echo c); (echo d));
    echo $( ( echo e ) | ( cat ));
    echo $(echo ${x:-$(echo ")")});
    echo $(echo "${x:-")"}");
    echo $(echo ${x/(/)})
}

== stderr 0

== merged 287
f () 
{ 
    echo $(echo \) ')' ")" ')' ${x#)} $((1)) "${y%%)*}");
    echo $(echo $(( (1+2) )) $( ( echo a )) $((echo b) ));
    echo $((echo c); (echo d));
    echo $( ( echo e ) | ( cat ));
    echo $(echo ${x:-$(echo ")")});
    echo $(echo "${x:-")"}");
    echo $(echo ${x/(/)})
}

== status 0
