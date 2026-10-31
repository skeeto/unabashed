f() {
    cat <<END_1
a
END_1
    cat << 'with space'
b
with space
    cat <<-'-'
c
-
    cat <<"$x"
d
$x
}
declare -f
