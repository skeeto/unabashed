f() {
    cat <<''
a $x

    echo one
    cat <<""
b $y

    echo two
    cat <<-''
	c

    echo three
}
declare -f
