f() {
    echo $(cat <<A <<B
a
A)
b
B
    echo $(cat <<E
y
EOF)
    echo $( (cat <<E
z
E) )
	echo $(cat <<-E
	w
	E)
    echo $(case x in x) cat <<E
v
E;; esac)
    echo $(( $(cat <<E
1
E) + 1 ))
    echo "$(cat <<'E'
u
E)"
}
declare -f
