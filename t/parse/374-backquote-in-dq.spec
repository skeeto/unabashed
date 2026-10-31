== stdout 135
f () 
{ 
    echo "a`echo "b c"`d" "`echo \"q\"`" "`echo ')'`" "x`echo \`y\``";
    echo "`echo a # b`" `echo c # d` "`echo ${x:-}`"
}

== stderr 0

== merged 135
f () 
{ 
    echo "a`echo "b c"`d" "`echo \"q\"`" "`echo ')'`" "x`echo \`y\``";
    echo "`echo a # b`" `echo c # d` "`echo ${x:-}`"
}

== status 0
