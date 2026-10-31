f() {
    echo ${x:-a\
b} "${y\
:-c}" ${z//\
/q} ${#\
w} $((1\
+2))
}
declare -f
