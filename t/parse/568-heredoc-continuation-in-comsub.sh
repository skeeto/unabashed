f() {
    echo $(cat <<E
foo\
E
E
)
    echo $(cat <<'E'
bar\
E
)
    cat <<E\
X
baz
EX
    echo $(cat <<-E
	qux\
	E
	E
)
}
declare -f
