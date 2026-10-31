f() {
    echo ${ cat <<E
x
E
}
    echo ${ { echo a; } ; }
    echo ${ sleep 1 & }
    echo ${ { echo b; } & }
    echo ${| REPLY=$(cat <<E
y
E
); }
    echo ${ cat <<E; }
z
E
    echo ${	echo tab;}
    echo ${
echo nl
}
}
declare -f
# nofixedpoint: bash prints ${ cat <<E ... E\n; } which it cannot reparse
