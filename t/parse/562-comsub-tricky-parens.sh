f() {
    echo $(echo \) ')' ")" $')' ${x#)} $((1)) "${y%%)*}")
    echo $(echo $(( (1+2) )) $( (echo a) ) $((echo b) ))
    echo $((echo c); (echo d))
    echo $( ( echo e ) | ( cat ) )
    echo $(echo ${x:-$(echo ")")})
    echo $(echo "${x:-")"}")
    echo $(echo ${x/(/)})
}
declare -f
