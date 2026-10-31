f() {
    cat <<A; echo $(echo x
echo y)
abody
A
    cat <<B; echo "q
r"
bbody
B
    cat <<C \
| cat
cbody
C
    cat <<D && {
dbody
D
echo in; }
    cat <<F; echo ${ echo g
}
fbody
F
}
declare -f
