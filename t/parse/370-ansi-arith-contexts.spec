== stdout 166
f () 
{ 
    echo $(( '1' + x )) "$(( '2' ))" $[ '3' ] "$(echo '	')" "`echo $'\t'`";
    (( y = '4' ));
    for ((i = '5'; i < 2; i++ ))
    do
        :;
    done
}

== stderr 0

== merged 166
f () 
{ 
    echo $(( '1' + x )) "$(( '2' ))" $[ '3' ] "$(echo '	')" "`echo $'\t'`";
    (( y = '4' ));
    for ((i = '5'; i < 2; i++ ))
    do
        :;
    done
}

== status 0
