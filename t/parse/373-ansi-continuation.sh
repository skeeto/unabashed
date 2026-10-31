f() {
    echo $'a\
b' $\
'c\td' $"e\
f"
}
declare -f
