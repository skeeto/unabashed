echo start
f() {
    cat <<A $(cat <<B
b
B
) x
a
A
    fi
}
echo notreached
