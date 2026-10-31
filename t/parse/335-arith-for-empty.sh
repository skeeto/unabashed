f() {
    for ((i=0;;)); do :; done
    for ((;i<3;)); do :; done
    for (( ; ; i++ )); do :; done
    for ((	;	;	)); do :; done
    for ((i=0, j=0; i<3; i++, j++))
    do
        :
    done
    for ((x;y;z)) ; do :; done
    for ((
i=0;
i<3;
i++
)); do :; done
}
declare -f
