f() {
    for ((i = 0; i < 10; i++)); do echo $i; done
    for ((;;)); do break; done
    for ((i=0,j=1; i<j; i++, j--))
    do
        :
    done
    for (( ; i < 3 ; )); do :; done
    for ((i=0; i<3; i++)) { echo $i; }
}
declare -f
