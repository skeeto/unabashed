== stdout 821
f () 
{ 
    x=$(echo a; echo b);
    echo "${x:-$(echo c)}" ${y:=$(echo d)} "${z/$(echo e)/$(echo f)}";
    echo $(( $(echo 1) )) $[ $(echo 2) ];
    (( $(echo 3) ));
    for ((i=$(echo 4); i<$(echo 5); i++ ))
    do
        :;
    done;
    [[ $(echo g) =~ $(echo h) ]];
    a[$(echo 6)]=$(echo i) b=($(echo j) [$(echo 7)]=k);
    echo ${a[$(echo 8)]} "${a[$(echo 9)]}";
    case $(echo l) in 
        $(echo m))

        ;;
    esac;
    for w in $(echo n) "$(echo o)";
    do
        :;
    done;
    cat < $(echo p) > "$(echo q)" <<< $(echo r);
    echo "$(echo s)" '$(echo   t)';
    echo `echo   u` "`echo   v`";
    echo <(echo w) >(echo x);
    echo ${ echo y; } ${|echo z; };
    echo "$(echo "$(echo nested)")";
    local v=$(echo aa) w="$(echo bb)";
    export e=$(echo cc);
    declare -a arr=($(echo dd))
}

== stderr 0

== merged 821
f () 
{ 
    x=$(echo a; echo b);
    echo "${x:-$(echo c)}" ${y:=$(echo d)} "${z/$(echo e)/$(echo f)}";
    echo $(( $(echo 1) )) $[ $(echo 2) ];
    (( $(echo 3) ));
    for ((i=$(echo 4); i<$(echo 5); i++ ))
    do
        :;
    done;
    [[ $(echo g) =~ $(echo h) ]];
    a[$(echo 6)]=$(echo i) b=($(echo j) [$(echo 7)]=k);
    echo ${a[$(echo 8)]} "${a[$(echo 9)]}";
    case $(echo l) in 
        $(echo m))

        ;;
    esac;
    for w in $(echo n) "$(echo o)";
    do
        :;
    done;
    cat < $(echo p) > "$(echo q)" <<< $(echo r);
    echo "$(echo s)" '$(echo   t)';
    echo `echo   u` "`echo   v`";
    echo <(echo w) >(echo x);
    echo ${ echo y; } ${|echo z; };
    echo "$(echo "$(echo nested)")";
    local v=$(echo aa) w="$(echo bb)";
    export e=$(echo cc);
    declare -a arr=($(echo dd))
}

== status 0
