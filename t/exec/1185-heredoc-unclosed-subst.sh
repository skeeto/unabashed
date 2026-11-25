# A here-document body can hold a ${ or $(( that never closes, which a
# word cannot. bash's scan for its end takes all the rest of the body,
# then reports a bad substitution: of the whole rest when the name is
# followed by other text (a newline at least), "no closing }" when an
# operator follows, once the name, indirection and subscript are read,
# and "no closing )" for $((. A bare ${ is a parse error. Prompt
# expansion (${x@P}) can end on one too: there each is only a bad
# substitution of the text.
# Each line is a script; ;; stands for a newline.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "${c//;;/$'\n'}"
    echo "status $?"
done <<'EOF'
cat <<X;;a ${x;;X;;echo after $?
cat <<X;;a ${x b;;c;;X;;echo after $?
cat <<X;;a ${x:-{};;X;;echo after $?
x=1; cat <<X;;${x;;y;;X
cat <<X;;a ${x;;b };;X
cat <<X;;${x[;;X
cat <<-X;;	${x;;	X
cat <<X;;\${x;;X
cat <<X;;${;;X
cat <<X;;${#;;X
cat <<X;;a $((1+(2);;X
cat <<X;;a $((1)) $((2;;X;;echo $?
cat <<X;;${x:-a;;X
cat <<X;;a ${x:;;X
cat <<X;;a ${#x:-b;;X
cat <<X;;a ${!x:-b;;X
x=y; cat <<X;;a ${!x:-b;;X
cat <<X;;a ${x[1]#b;;X
cat <<X;;a ${1%b;;X
cat <<X;;a ${x b:-c;;X
cat <<X;;a ${x/b;;X
cat <<X;;a ${x@;;X
a=(1); cat <<X;;a ${a[1/0]:-b;;X
set -u; cat <<X;;a ${u%b;;X
x='${a'; echo "<${x@P}>"; echo $?
a=1; x='a ${a:-b'; echo "<${x@P}>"; echo $?
x='${a b'; echo "<${x@P}>"; echo $?
EOF
