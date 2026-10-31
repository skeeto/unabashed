f() {
	cat <<-A <<B
	a body
	A
	b body
B
}
declare -f
