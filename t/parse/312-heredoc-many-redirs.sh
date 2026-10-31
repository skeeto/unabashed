f() {
    cat <<A >out <<-B 2>&1 <<'C' {v}<<D 3<<<str
a
A
	b
	B
c
C
d
D
    echo after
}
declare -f
