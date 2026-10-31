f() {
    echo ${ cat <<E
x
E }
	echo ${ cat <<-E
	y
	E}
    echo "${ cat <<E
w
E }"
}
declare -f
