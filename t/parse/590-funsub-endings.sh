f() {
    echo ${ echo a
}
    echo ${ echo b;
}
    echo ${ (echo c) }
    echo ${ case x in x) ;; esac }
    echo ${ echo d; # comment
}
    echo ${ { echo e; } }
    echo ${ echo f &
}
    echo ${ echo g


}
    echo ${ x=1; }
    echo ${ x=1
}
    echo ${ [[ -n h ]] }
    echo ${ (( 1 )) }
    echo ${ for i in 1; do :; done }
    echo ${ g() { :; } }
    echo ${| echo i | cat; }
    echo ${ ! true; }
    echo ${ time; }
}
declare -f
