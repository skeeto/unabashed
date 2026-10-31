f() {
	cat <<-EOF
		tabbed
	  mixed
    spaces kept
	EOF
	cat <<-'Q'
	$literal
	Q
}
declare -f
