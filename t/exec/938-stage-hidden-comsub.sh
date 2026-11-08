# An external pipeline stage whose words run code hidden in a value
# (an arithmetic subscript, an indirection, a nameref) expands while
# the stages after it run: here its comsub waits on the last stage
mkfifo p
echo one >f1
a=(/dev/null f1)
i='a[$(cat p >/dev/null; echo 1)]'
echo in | cat - ${a[i]} | { echo go >p; cat; }
echo "${PIPESTATUS[*]}"
x='a[$(cat p >/dev/null; echo 1)]'
echo in | cat - ${!x} | { echo go >p; cat; }
declare -n r='a[$(cat p >/dev/null; echo 1)]'
echo in | cat - $r | { echo go >p; cat; }
echo in | cat - "${a[@]:i:1}" | { echo go >p; cat; }
echo in | cat - ${a[$((i))]} | { echo go >p; cat; }
echo done
