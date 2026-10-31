shopt -s extglob
f() {
    echo $(echo @(a|b) !(c) +(d|e)f *(g) ?(h))
    echo $(case x in @(a|b)) echo 1;; !(*\)*)) echo 2;; esac)
    echo "$(ls +([0-9]).txt)"
    echo `echo @(x|y)`
    echo ${ echo *(z); }
    [[ $(echo a) == @(a|$(echo b)) ]]
}
declare -f
