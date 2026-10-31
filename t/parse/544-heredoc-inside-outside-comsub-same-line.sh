f() {
    cat <<A $(cat <<B
b
B
) more
a
A
    cat $(cat <<C
c
C
) <<D; echo $(cat <<E
e
E
)
d
D
    echo end
}
declare -f
