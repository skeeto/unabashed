f() {
    echo ${ case x in }) echo a;; esac; }
    echo ${ echo }; }
    echo ${ echo \}; }
    echo "${ echo "}"; }"
    echo ${ g() { echo h; }; g; }
    echo ${ if true; then echo i; fi }
    echo ${ (echo j) }
    echo ${ echo $(echo k) ${ echo l; }; }
    echo ${|
        REPLY=m
    }
}
declare -f
