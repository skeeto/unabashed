f() {
    cat <<E $(cat <<E
inner
E
)
outer
E
	echo $(cat <<-E
	x
	E
	)
    echo $(cat <<A $(cat <<B $(cat <<C
c
C
)
b
B
)
a
A
)
    x=$( { cat; } <<E | cat
y
E
)
}
declare -f
