f() {
    x=$(echo $(cat <<A
inner
A
) outer)
    y=$(if cat <<B; then
b
B
echo t; fi)
}
declare -f
